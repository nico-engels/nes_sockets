#include "win_socket.h"

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <thread>
#include "cfg.h"
#include "net_exc.h"
#include "socket_util.h"

// WINSOCK2 API
#ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace nes;
using namespace nes::net;

namespace nes::so {

  // Global instance counter Init/Destroy
  atomic<unsigned> wsa_lib_instances { 0 };

  // WSA (Windows Sockets API) Initialization/Finalization
  void WSA_init();
  void WSA_finalizer();

  // Aux error messages
  string msg_err_str(int);

  // Aux RAII temporary handle
  namespace {
    class socket_raii {
      SOCKET m_handle;
    public:
      socket_raii(SOCKET handle) : m_handle { handle } {};
      ~socket_raii() { if (m_handle != INVALID_SOCKET) closesocket(m_handle); };
      SOCKET handle() const { return m_handle; };
      SOCKET release() { SOCKET s = m_handle; m_handle = INVALID_SOCKET; return s; };
    };

    // Aux RAII addrinfo
    class addrinfo_raii {
      addrinfo* m_handle;
     public:
      addrinfo_raii(addrinfo* handle) : m_handle { handle } {};
      ~addrinfo_raii() { if (m_handle) freeaddrinfo(m_handle); };
    };
  }

  win_socket::win_socket()
    : m_winsocket { INVALID_SOCKET }
  {
    WSA_init();
  }

  win_socket::win_socket(win_socket&& other) noexcept
    : m_winsocket { other.m_winsocket }
    , m_ipv4_address { move(other.m_ipv4_address) }
    , m_ipv4_port { other.m_ipv4_port }
  {
    other.m_winsocket = INVALID_SOCKET;

    WSA_init();
  }

  win_socket& win_socket::operator=(win_socket&& other) noexcept
  {
    m_winsocket = other.m_winsocket;
    other.m_winsocket = INVALID_SOCKET;

    m_ipv4_address = move(other.m_ipv4_address);
    m_ipv4_port = other.m_ipv4_port;

    return *this;
  }

  win_socket::~win_socket()
  {
    // If valid cleanup
    if (m_winsocket != INVALID_SOCKET)
    {
      shutdown(m_winsocket, SD_BOTH);
      closesocket(m_winsocket);
    }
    WSA_finalizer();
  }

  const string& win_socket::ipv4_address() const
  {
    return m_ipv4_address;
  }

  unsigned win_socket::ipv4_port() const
  {
    return m_ipv4_port;
  }

  win_socket::native_handle_type win_socket::native_handle() const
  {
    return m_winsocket;
  }

  void win_socket::listen(unsigned port)
  {
    if (m_winsocket != INVALID_SOCKET)
      throw nes_exc { "Socket already configured." };

    socket_raii sock_serv { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };

    if (sock_serv.handle() == INVALID_SOCKET)
      throw nes_exc { "WSA error on create socket." };

    // Address Resolution
    addrinfo addr_res_cfg;

    memset(&addr_res_cfg, 0, sizeof(addr_res_cfg));

    addr_res_cfg.ai_family = AF_INET;
    addr_res_cfg.ai_socktype = SOCK_STREAM;
    addr_res_cfg.ai_protocol = IPPROTO_TCP;
    addr_res_cfg.ai_flags = AI_PASSIVE;

    addrinfo *addr_res;
    if (getaddrinfo(NULL, to_string(port).c_str(), &addr_res_cfg, &addr_res))
      throw nes_exc { "WSA Address resolution error." };

    addrinfo_raii addr { addr_res };

    if (::bind(sock_serv.handle(), addr_res->ai_addr, static_cast<int>(addr_res->ai_addrlen)))
      throw nes_exc { "Socket cannot bind IPV4 port {}.", port };

    // Non-blocking socket
    u_long mode = 1;
    if (ioctlsocket(sock_serv.handle(), FIONBIO, &mode))
      throw nes_exc { "Socket error setting a listening socket to non-blocking." };

    // Start listing, but not block
    // SOMAXCONN = Maximun number of client in queue
    if (::listen(sock_serv.handle(), SOMAXCONN))
      throw nes_exc { "Socket error listen on IPv4 port {}.", port };

    // All ok, can set the class
    m_winsocket = sock_serv.release();
    m_ipv4_address = "0.0.0.0";
    m_ipv4_port = port;
  }

  bool win_socket::is_connected() const
  {
    return m_winsocket != INVALID_SOCKET && m_ipv4_address != "0.0.0.0" && m_ipv4_port != 0;
  }

  bool win_socket::is_listening() const
  {
    return m_winsocket != INVALID_SOCKET && m_ipv4_address == "0.0.0.0" && m_ipv4_port != 0;
  }

  bool win_socket::has_client()
  {
    if (!this->is_listening())
      return false;

    FD_SET fdset_socket;
    static timeval timeout { 0L, 0L };

    // Init
    FD_ZERO(&fdset_socket);
    FD_SET(m_winsocket, &fdset_socket);

    // OS check if is any client
    if (select(0, &fdset_socket, nullptr, nullptr, &timeout) == SOCKET_ERROR)
      return false;

    // If socket stay in the vector there is a client
    return FD_ISSET(m_winsocket, &fdset_socket) != 0;
  }

  optional<win_socket> win_socket::accept()
  {
    if (!this->is_listening())
      throw nes_exc { "Socket is not listing, cannot accept connection." };

    struct sockaddr_in client_info = { 0, 0, 0, 0, 0, 0, 0 };
    int size = sizeof(client_info);

    // Try to get some client
    SOCKET socket_cli = ::accept(m_winsocket, reinterpret_cast<sockaddr*>(&client_info), &size);
    if (socket_cli == INVALID_SOCKET)
    {
      // Return nullopt only if there is no client
      if (WSAGetLastError() == WSAEWOULDBLOCK)
        return nullopt;
      else
        throw nes_exc { "Socket error accept." };
    }
    else
    {
      // New client received
      win_socket ret;
      ret.m_winsocket = socket_cli;

      // IPv4 Identification
      // xxx.xxx.xxx.xxx:yyyy
      wstring ip_port(32, wchar_t {});
      DWORD size_str_ip = static_cast<DWORD>(ip_port.size());
      if (WSAAddressToStringW(reinterpret_cast<sockaddr*>(&client_info), size, nullptr,
                              ip_port.data(), &size_str_ip))
        throw nes_exc { "Socket IPv4 identification fail." };

      ip_port.resize(size_str_ip);
      ret.m_ipv4_address.resize(size_str_ip);
      for (const auto ch : ip_port)
        ret.m_ipv4_address.push_back(static_cast<char>(ch));

      auto pos = ret.m_ipv4_address.find(':');
      if (pos == string::npos)
        throw nes_exc { "Socket IPv4 port identification fail." };

      ret.m_ipv4_port = stoi(ret.m_ipv4_address.substr(pos + 1));
      ret.m_ipv4_address.erase(pos);

      // Set as non blocking
      u_long mode = 1;
      if (ioctlsocket(ret.m_winsocket, FIONBIO, &mode))
        throw nes_exc { "Socket error setting a listening socket to non-blocking." };

      return { move(ret) };
    }
  }

  void win_socket::connect(string addr, unsigned port)
  {
    if (m_winsocket != INVALID_SOCKET)
      throw nes_exc { "Socket already configured." };

    socket_raii socket_cli { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };

    if (socket_cli.handle() == INVALID_SOCKET)
      throw nes_exc { "WSA error on create socket." };

    // Address Resolution
    struct addrinfo addr_res_cfg;

    memset(&addr_res_cfg, 0, sizeof(addr_res_cfg));

    addr_res_cfg.ai_family = AF_INET;
    addr_res_cfg.ai_socktype = SOCK_STREAM;
    addr_res_cfg.ai_protocol = IPPROTO_TCP;
    addr_res_cfg.ai_flags = AI_PASSIVE;

    struct addrinfo *addr_res;
    if (getaddrinfo(addr.data(), to_string(port).c_str(), &addr_res_cfg, &addr_res))
      throw nes_exc { "WSA Address resolution error." };

    addrinfo_raii addr_inf_scope { addr_res };

    // Resolved IPv4
    string ip;
    ip.resize(INET_ADDRSTRLEN);

    struct sockaddr_in *addr4 = reinterpret_cast<struct sockaddr_in*>(addr_res->ai_addr);
    const char* res = inet_ntop(AF_INET, &addr4->sin_addr, ip.data(), INET_ADDRSTRLEN);

    if (!res)
      throw nes_exc { "WSA extract IPv4 address error." };

    ip.resize(strlen(res));

    if (::connect(socket_cli.handle(), addr_res->ai_addr, static_cast<int>(addr_res->ai_addrlen)))
      throw nes_exc { "Socket error connecting at address '{}({}):{}'.", addr, ip, port };

    // Non blocking socket
    u_long mode = 1;
    if (ioctlsocket(socket_cli.handle(), FIONBIO, &mode))
      throw nes_exc { "Socket error setting the socket to non-blocking." };

    // All ok, can set the class
    m_winsocket = socket_cli.release();
    m_ipv4_address = move(ip);
    m_ipv4_port = port;
  }

  void win_socket::disconnect()
  {
    if (this->is_connected())
    {
      shutdown(m_winsocket, SD_BOTH);
      closesocket(m_winsocket);

      m_winsocket = INVALID_SOCKET;
      m_ipv4_address = { "0.0.0.0" };
      m_ipv4_port = 0;
    }
  }

  void win_socket::send(span<const std::byte> data_span)
  {
    if (!this->is_connected())
      throw nes_exc { "Socket is not connected, cannot send data." };

    if (data_span.size() == 0)
      return;

    // Send the data in cfg::net::packet_size chunks
    size_t retry_count = 0;
    auto interval = cfg::net::wait_io_step_min;
    while (data_span.size())
    {
      auto chunk_size = min(data_span.size(), cfg::net::packet_size);
      auto chunk = data_span.first(chunk_size);

      auto ret = ::send(m_winsocket, reinterpret_cast<const char*>(chunk.data()), chunk.size(), 0);
      if (ret == SOCKET_ERROR)
      {
        if (retry_count < cfg::net::io_max_retry && errno == WSAEWOULDBLOCK)
        {
          // As get more tries increase the wait
          this_thread::sleep_for(interval);
          retry_count++;
          interval = calculate_interval_retry(retry_count);
        }
        else if (errno == WSAECONNABORTED)
          throw socket_disconnected { "Socket closed by destination." };
        else
          throw nes_exc { "Error on socket send data. Error: {}", msg_err_str(errno) };
      }
      else
      {
        chunk_size = ret;
        retry_count = 0;
        interval = cfg::net::wait_io_step_min;
      }

      // Shrink the span
      data_span = data_span.last(data_span.size() - chunk_size);
    }

  }

  vector<std::byte> win_socket::receive()
  {
    if (!this->is_connected())
      throw nes_exc { "Socket is not connected, cannot receive data." };

    array<std::byte, cfg::net::packet_size> packet_buffer;
    vector<std::byte> ret;

    while (true)
    {
      auto qtde = recv(m_winsocket, reinterpret_cast<char*>(packet_buffer.data()),
        static_cast<int>(packet_buffer.size()), 0);
      if (qtde == SOCKET_ERROR)
      {
        auto erro = WSAGetLastError();

        // No data to receive, but the connection is active
        if (erro == WSAEWOULDBLOCK)
          break;
        else
          throw nes_exc { "Error on socket data receive. Error: {}.", msg_err_str(erro) };
      }
      else if (qtde == 0)
      {
        // Closed socket, if there is data breaks
        if (ret.size() > 0)
          break;

        throw socket_disconnected { "Socket closed normally." };
      }

      // Collect the data
      ret.insert(ret.end(), packet_buffer.begin(), packet_buffer.begin() + qtde);
    }

    return ret;
  }

  void WSA_init()
  {
    // Initialize on first instance
    if (wsa_lib_instances++ == 0)
    {
      WSADATA wsaData;

      // Version da WS2_32.Lib 2.2
      auto retorno = WSAStartup(MAKEWORD(2, 2), &wsaData);
      if (retorno)
        throw nes_exc { "WSA init error: {}.", msg_err_str(retorno) };

      // Only 2.2 version suported by nes_sockets
      if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
      {
        WSACleanup();
        throw nes_exc { "WSA required version: 2.2. Version in this system: {}.{}!",
          LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion) };
      }
    }
  }

  void WSA_finalizer()
  {
    if (--wsa_lib_instances == 0)
      WSACleanup();
  }

  string msg_err_str(int err_code)
  {
    array<char, 1024> buff {};
    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
      err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buff.data(), static_cast<DWORD>(buff.size()), NULL) == 0)
      return to_string(err_code) + " - msg_err_str! Last WSA Error: " + to_string(WSAGetLastError());
    else
      return to_string(err_code) + " - " + buff.data();
  }

}