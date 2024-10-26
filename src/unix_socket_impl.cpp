module;

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>

module unix_socket;

import <array>;
import <atomic>;
import <chrono>;
import <cstring>;
import <functional>;
import <stdexcept>;
import <string>;
import <thread>;

import cfg;
import nes_exc;
import net_exc;
import socket_util;

using namespace std;
using namespace std::chrono_literals;
using namespace nes::net;
using namespace nes;

namespace nes::so {

  constexpr int SOCKET_INVALID = -1;
  constexpr int SOCKET_ERROR = -1;

  unix_socket::unix_socket()
    : m_unix_sd { SOCKET_INVALID }
  {

  }

  unix_socket::~unix_socket()
  {
    // If valid cleanup
    if (m_unix_sd != SOCKET_INVALID)
    {
      shutdown(m_unix_sd, SHUT_RDWR);
      close(m_unix_sd);
    }
  }

  unix_socket::unix_socket(unix_socket&& other) noexcept
    : m_unix_sd { other.m_unix_sd }
    , m_ipv4_address { move(other.m_ipv4_address) }
    , m_ipv4_port { other.m_ipv4_port }
  {
    other.m_unix_sd = SOCKET_INVALID;
  }

  unix_socket& unix_socket::operator=(unix_socket&& other) noexcept
  {
    m_unix_sd = other.m_unix_sd;
    other.m_unix_sd = SOCKET_INVALID;

    m_ipv4_address = move(other.m_ipv4_address);
    m_ipv4_port = other.m_ipv4_port;

    return *this;
  }

  const string& unix_socket::ipv4_address() const
  {
    return m_ipv4_address;
  }

  unsigned unix_socket::ipv4_port() const
  {
    return m_ipv4_port;
  }

  unix_socket::native_handle_type unix_socket::native_handle() const
  {
    return m_unix_sd;
  }

  void unix_socket::listen(unsigned port)
  {
    if (m_unix_sd != SOCKET_INVALID)
      throw nes_exc { "Socket already configured." };

    unique_ptr<int, function<void(int*)>> sock_serv = {
      new int { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) },
      [](int* p) { if (*p != SOCKET_INVALID) { close(*p); } delete p; }
    };

    if (*sock_serv == SOCKET_INVALID)
      throw nes_exc { "Error on create Socket in linux syscall." };

    // Address Resolution
    struct sockaddr_in addr_res;

    addr_res.sin_family = AF_INET;
    addr_res.sin_addr.s_addr = INADDR_ANY;
    addr_res.sin_port = htons(static_cast<short unsigned>(port));

    if (bind(*sock_serv, reinterpret_cast<struct sockaddr*>(&addr_res), sizeof(addr_res)) < 0)
      throw nes_exc { "Socket cannot bind IPV4 port {}. Error {}: '{}'.", port, errno, strerror(errno) };

    // Non-blocking socket
    int marks = fcntl(*sock_serv, F_GETFL, 0);
    if (marks < 0 || fcntl(*sock_serv, F_SETFL, marks | O_NONBLOCK) != 0)
      throw nes_exc { "Socket error setting a listening socket to non-blocking." };

    // Start listing, but not block
    // SOMAXCONN = Maximun number of client in queue
    if (::listen(*sock_serv, SOMAXCONN) < 0)
      throw nes_exc { "Socket error listen on IPv4 port {}.", port };

    // All ok, can set the class
    m_unix_sd = *sock_serv.release();
    m_ipv4_address = "0.0.0.0";
    m_ipv4_port = port;
  }

  bool unix_socket::is_connected() const
  {
    return m_unix_sd != SOCKET_INVALID && m_ipv4_address != "0.0.0.0" && m_ipv4_port != 0;
  }

  bool unix_socket::is_listening() const
  {
    return m_unix_sd != SOCKET_INVALID && m_ipv4_address == "0.0.0.0" && m_ipv4_port != 0;
  }

  bool unix_socket::has_client()
  {
    if (!this->is_listening())
      return false;

    // Init, uses the poll functionality
    pollfd fd_sock;
    fd_sock.fd = m_unix_sd;
    fd_sock.events = POLLIN;

    if (poll(&fd_sock, 1, 0) <= 0)
     return false;

    if (fd_sock.revents & POLLIN)
      return true;

    return false;
  }

  optional<unix_socket> unix_socket::accept()
  {
    if (!this->is_listening())
      throw nes_exc { "Socket is not listing, cannot accept connection." };

    sockaddr_in client_info {};
    socklen_t size = sizeof(client_info);

    // Try to get some client
    int socket_cli = ::accept(m_unix_sd, reinterpret_cast<sockaddr*>(&client_info), &size);
    if (socket_cli == SOCKET_INVALID)
    {
      // Return nullopt only if there is no client
      if (errno == EWOULDBLOCK)
        return nullopt;
      else
        throw nes_exc { "Socket error accept." };
    }
    else
    {
      // New client received
      unix_socket ret;

      ret.m_unix_sd = socket_cli;
      ret.m_ipv4_address = inet_ntoa(client_info.sin_addr);
      ret.m_ipv4_port = ntohs(client_info.sin_port);

      // Set as non blocking
      int marks = fcntl(socket_cli, F_GETFL, 0);
      if (marks < 0 || fcntl(socket_cli, F_SETFL, marks | O_NONBLOCK) != 0)
        throw nes_exc { "Socket error setting a listening socket to non-blocking." };

      return { move(ret) };
    }
  }

  void unix_socket::connect(string addr, unsigned port)
  {
    if (m_unix_sd != SOCKET_INVALID)
      throw nes_exc { "Socket already configured." };

    unique_ptr<int, function<void(int*)>> socket_cli = {
      new int { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) },
      [](int* p) { if (*p != SOCKET_INVALID) { close(*p); } delete p; }
    };

    if (*socket_cli == SOCKET_INVALID)
      throw nes_exc { "Error on create Socket in linux syscall." };

    // Address Resolution
    struct addrinfo addr_res_cfg;

    memset(&addr_res_cfg, 0, sizeof(addr_res_cfg));

    addr_res_cfg.ai_family = AF_INET;
    addr_res_cfg.ai_socktype = SOCK_STREAM;
    addr_res_cfg.ai_protocol = IPPROTO_TCP;
    addr_res_cfg.ai_flags = AI_PASSIVE;

    struct addrinfo *addr_res;
    if (getaddrinfo(addr.data(), to_string(port).c_str(), &addr_res_cfg, &addr_res))
      throw nes_exc { "Address resolution error." };

    unique_ptr<struct addrinfo, function<void(struct addrinfo*)>> endResolvidoPtr {
      addr_res,
      [](struct addrinfo* p) { freeaddrinfo(p); }
    };

    // Resolved IPv4
    string ip;
    ip.resize(INET_ADDRSTRLEN);

    struct sockaddr_in *addr4 = reinterpret_cast<struct sockaddr_in*>(
      reinterpret_cast<void*>(addr_res->ai_addr));
    const char* res = inet_ntop(AF_INET, &addr4->sin_addr, ip.data(), INET_ADDRSTRLEN);

    if (!res)
      throw nes_exc { "Adress IPv4 extraction error." };

    ip.resize(strlen(res));

    if (::connect(*socket_cli, addr_res->ai_addr, static_cast<socklen_t>(addr_res->ai_addrlen)) != 0)
      throw nes_exc { "Socket error connecting at address '{}({}):{}'.", addr, ip, port };

    // Non blocking socket
    int marks = fcntl(*socket_cli, F_GETFL, 0);
    if (marks < 0 || fcntl(*socket_cli, F_SETFL, marks | O_NONBLOCK) != 0)
      throw nes_exc { "Socket error setting the socket to non-blocking." };

    // All ok, can set the class
    m_unix_sd = *socket_cli.release();
    m_ipv4_address = move(ip);
    m_ipv4_port = port;
  }

  void unix_socket::disconnect()
  {
    if (this->is_connected())
    {
      shutdown(m_unix_sd, SHUT_RDWR);
      close(m_unix_sd);

      m_unix_sd = SOCKET_INVALID;
      m_ipv4_address = "0.0.0.0";
      m_ipv4_port = 0;
    }
  }

  void unix_socket::send(span<const byte> data_span)
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

      auto ret = ::send(m_unix_sd, reinterpret_cast<const char*>(chunk.data()), chunk.size(), 0);
      if (ret == -1)
      {
        if (retry_count < cfg::net::io_max_retry && errno == EWOULDBLOCK)
        {
          // As get more tries increase the wait
          this_thread::sleep_for(interval);
          retry_count++;
          interval = calculate_interval_retry(retry_count);
        }
        else
          throw nes_exc { "Error on socket send data. Error: {} - '{}'!", errno, strerror(errno) };
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

  vector<byte> unix_socket::receive()
  {
    // Validação
    if (!this->is_connected())
      throw nes_exc { "Socket is not connected, cannot receive data." };

    array<byte, cfg::net::packet_size> packet_buffer;
    vector<byte> ret;

    while (true)
    {
      auto qtde = recv(m_unix_sd, reinterpret_cast<void*>(packet_buffer.data()), packet_buffer.size(), 0);

      if (qtde == SOCKET_ERROR)
      {
        // No data to receive, but the connection is active
        if (errno == EWOULDBLOCK)
          break;
        else
          throw nes_exc { "Error on socket data receive. Error: {}.", errno };
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

}
