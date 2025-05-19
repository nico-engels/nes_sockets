module;

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

export module tls_socket;

import std;

import cfg;
import nes_exc;
import socket;
import socket_util;

// Declaração
export namespace nes::net {

  class tls_socket final
  {
    // Socket and OpenSSL handler
    socket m_sock;
    SSL* m_sock_ssl { nullptr };

    // TLS Handshake state
    enum class handshake_state { connect, accept, ok };
    handshake_state m_handshake { handshake_state::connect };
    void handshake();

  public:
    tls_socket();
    tls_socket(SSL*, socket);

    // Constructor (Host, port)
    tls_socket(std::string, unsigned);

    ~tls_socket();

    tls_socket(const tls_socket&) = delete;
    tls_socket(tls_socket&&);

    const tls_socket& operator=(const tls_socket&) const = delete;
    tls_socket& operator=(tls_socket&&);

    // Connection (Host, port)
    void connect(std::string, unsigned);
    void disconnect();

    // TLS Extensions
    // Virtual host (same host many sites)
    void tls_ext_host_name(std::string);

    // IPv4 Connection Data
    const std::string& ipv4_address() const;
    unsigned ipv4_port() const;
    bool is_connected() const;

    std::string cipher() const;
    std::string tls_protocol() const;

    // I/O basic functions (binary or binary char)
    void send(std::span<const std::byte>);
    void send(std::string_view);
    [[nodiscard]] std::vector<std::byte> receive();

    // I/O basic utilities
    // Where exists the time_expire and/or max_size are used as maximum threasholds
    // Spin receiving data until finds the delim arg, return the data and pos of delim in data
    template <class R, class P = std::ratio<1>>
    [[nodiscard]] std::pair<std::vector<std::byte>, std::size_t>
    receive_until_delimiter(std::span<const std::byte> delim, std::chrono::duration<R, P> time_expire,
      std::size_t max_size);

    // Read data until receive the exact_size number of bytes
    template <class R, class P = std::ratio<1>>
    [[nodiscard]] std::vector<std::byte> receive_until_size(std::size_t exact_size,
      std::chrono::duration<R, P> time_expire);

    // Read data until receive the >= at_least_size bytes
    template <class R, class P = std::ratio<1>>
    [[nodiscard]] std::vector<std::byte> receive_at_least(std::size_t at_least_size,
      std::chrono::duration<R, P> time_expire);

    // Complete the data arg until the data.size() is equals arg total_size
    template <class R, class P>
    void receive_remaining(std::vector<std::byte>& data, size_t total_size, std::chrono::duration<R, P> time_expire);

    // Make manual TLS handshake (auxiliary function same thread connection)
    friend void same_thread_handshake(tls_socket&, tls_socket&);
  };

  // Init/Destroy flag
  std::once_flag init_lib;
  void initialize_OpenSSL();

  SSL_CTX *openssl_ctx();
  void openssl_ctx_free();

  // Aux RAII temporary handle
  class sockssl_rai final
  {
    SSL *m_sock_ssl { nullptr };

  public:
    sockssl_rai(SSL* sock) : m_sock_ssl { sock } {};
    ~sockssl_rai() { if (m_sock_ssl) SSL_free(m_sock_ssl); };

    sockssl_rai(const sockssl_rai&) = delete;

    SSL* handle() { return m_sock_ssl; };
    SSL* release() { SSL *ret = m_sock_ssl; m_sock_ssl = nullptr; return ret; };
  };

  class ctxssl_ini final
  {
    bool m_is_init { false };
  public:
    ctxssl_ini() { openssl_ctx(); }

    ~ctxssl_ini() { if (!m_is_init) openssl_ctx_free(); }

    void set_initialized() { m_is_init = true; };
  };
}

// Definição
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace nes;

namespace nes::net {

  tls_socket::tls_socket()
  {
    call_once(init_lib, initialize_OpenSSL);

    openssl_ctx();
  }

  tls_socket::tls_socket(SSL* ssl, socket s)
    : m_sock { move(s) }
    , m_sock_ssl { ssl }
    , m_handshake { handshake_state::accept }
  {
    openssl_ctx();
  }

  tls_socket::tls_socket(string ip, unsigned port)
  {
    call_once(init_lib, initialize_OpenSSL);

    ctxssl_ini ini;

    this->connect(ip, port);

    ini.set_initialized();
  }

  tls_socket::tls_socket(tls_socket&& other)
    : m_sock { move(other.m_sock) }
    , m_sock_ssl { other.m_sock_ssl }
    , m_handshake { other.m_handshake }
  {
    openssl_ctx();

    other.m_sock_ssl = nullptr;
  }

  tls_socket& tls_socket::operator=(tls_socket&& other)
  {
    swap(m_sock, other.m_sock);
    swap(m_sock_ssl, other.m_sock_ssl);
    swap(m_handshake, other.m_handshake);

    return *this;
  }

  tls_socket::~tls_socket()
  {
    this->disconnect();

    openssl_ctx_free();
  }

  void tls_socket::connect(string addr, unsigned port)
  {
    if (m_sock_ssl)
      throw nes_exc { "TLS-Socket already configured." };

    // First create the native socket, the tls protocol is layered
    socket s (move(addr), port);

    // OpenSSL hangler
    SSL *sock_ssl = SSL_new(openssl_ctx());
    if (!sock_ssl)
      throw nes_exc { "Not possible alocate the OpenSSL client context." };
    sockssl_rai ssock_ssl(sock_ssl);

    // Bind SSL with the nes_socket
    int ret = SSL_set_fd(ssock_ssl.handle(), static_cast<int>(s.native_handle()));
    if (ret != 1)
      throw nes_exc { "Can not bind the SSL handle with native socket." };

    // All ok, can set the class
    m_sock_ssl = ssock_ssl.release();
    m_sock = move(s);
  }

  void tls_socket::disconnect()
  {
    if (m_sock_ssl)
    {
      SSL_free(m_sock_ssl);
      m_sock_ssl = nullptr;

      m_sock.disconnect();
      m_handshake = handshake_state::connect;
    }
  }

  void tls_socket::handshake()
  {
    milliseconds interval = cfg::net::wait_io_step_min;
    size_t retry_count = 0;

    // Handshake process
    int ret = -1;
    do {
      switch (m_handshake)
      {
        case handshake_state::connect:
          ret = SSL_connect(m_sock_ssl);
          break;

        case handshake_state::accept:
          ret = SSL_accept(m_sock_ssl);
          break;

        case handshake_state::ok:
          throw nes_exc { "Handshake already over." };
      }

      if (ret == 0)
        throw nes_exc { "Handshake error." };
      else if (ret == -1)
      {
        auto errcode = SSL_get_error(m_sock_ssl, -1);
        switch(errcode)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
          {
            this_thread::sleep_for(interval);

            if (retry_count >= cfg::net::io_max_retry)
              throw nes_exc { "Handshake timeout." };

            interval = calculate_interval_retry(++retry_count);
            break;
          }
          default:
          {
            vector<decltype(errcode)> erros;
            string msg = "Error while making the handshake!\n";
            erros.push_back(errcode);

            // Collect all the errors and create the error message
            while ((errcode = static_cast<decltype(errcode)>(ERR_get_error())) != 0)
              erros.push_back(errcode);

            for (const auto erro : erros)
              msg += to_string(erro) + " " + ERR_error_string(static_cast<unsigned long>(erro), NULL);

            throw nes_exc { msg };
          }
        }
      }
    } while (ret != 1);

    m_handshake = handshake_state::ok;
  }

  void tls_socket::tls_ext_host_name(string host)
  {
    // Special TLS Protocol extensions
    if (m_sock_ssl)
      SSL_ctrl(m_sock_ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, host.data());
  }

  const string& tls_socket::ipv4_address() const
  {
    return m_sock.ipv4_address();
  }

  unsigned tls_socket::ipv4_port() const
  {
    return m_sock.ipv4_port();
  }

  bool tls_socket::is_connected() const
  {
    return m_sock_ssl != nullptr;
  }

  string tls_socket::cipher() const
  {
    if (m_sock_ssl)
      return string { SSL_get_cipher(m_sock_ssl) };
    else
      return string {};
  }

  string tls_socket::tls_protocol() const
  {
    if (m_sock_ssl)
      return string { SSL_get_version(m_sock_ssl) };
    else
      return string {};
  }

  void tls_socket::send(span<const std::byte> data_span)
  {
    if (!m_sock.is_connected())
      throw nes_exc { "The TLS socket is not connected." };

    if (m_handshake != handshake_state::ok)
      this->handshake();

    milliseconds interval = cfg::net::wait_io_step_min;
    size_t retry_count = 0;
    int ret;
    do {
      ret = SSL_write(m_sock_ssl, data_span.data(), static_cast<int>(data_span.size()));
      if (ret > 0)
      {
        if (static_cast<size_t>(ret) != data_span.size())
          throw nes_exc { "Can not send all the data." };
      }
      else
      {
        auto coderr = SSL_get_error(m_sock_ssl, ret);
        switch(coderr)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
          {
            this_thread::sleep_for(interval);

            if (retry_count >= cfg::net::io_max_retry)
              throw nes_exc { "Timeout sending data." };

            interval = calculate_interval_retry(++retry_count);

            break;
          }
          default:
            throw nes_exc { "Error sending data! Cod.: {}", coderr };
        }
      }
    } while (ret <= 0);
  }

  void tls_socket::send(string_view data_str)
  {
    this->send(as_bytes(span { data_str.begin(), data_str.end() }));
  }

  vector<std::byte> tls_socket::receive()
  {
    if (!m_sock.is_connected())
      throw nes_exc { "The TLS socket is not connected." };

    if (m_handshake != handshake_state::ok)
      this->handshake();

    array<std::byte, cfg::net::packet_size> packet_buffer;
    vector<std::byte> ret;

    while (true)
    {
      int res = SSL_read(m_sock_ssl, packet_buffer.data(), static_cast<int>(packet_buffer.size()));
      if (res > 0)
        ret.insert(ret.end(), packet_buffer.begin(), packet_buffer.begin() + static_cast<size_t>(res));
      else
      {
        auto coderr = SSL_get_error(m_sock_ssl, res);
        switch(coderr)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            break;
          default:
          {
            if (ret.size())
              break;

            // Check if the underlying socket has clossed normally
            if (coderr == SSL_ERROR_SYSCALL && ERR_get_error() == 0)
              static_cast<void>(m_sock.receive());

            throw nes_exc { "Error receiving data! Cod.: {}", coderr };
          }
        }
        break;
      }
    }

    return ret;
  }

  template <class R, class P>
  pair<vector<std::byte>, size_t>
  tls_socket::receive_until_delimiter(span<const std::byte> delim, duration<R, P> time_expire, size_t max_size)
  {
    using nes::net::receive_until_delimiter;
    return receive_until_delimiter(*this, delim, time_expire, max_size);
  }

  template <class R, class P>
  vector<std::byte> tls_socket::receive_until_size(size_t exact_size, duration<R, P> time_expire)
  {
    using nes::net::receive_until_size;
    return receive_until_size(*this, exact_size, time_expire);
  }

  template <class R, class P>
  vector<std::byte> tls_socket::receive_at_least(size_t at_least_size, duration<R, P> time_expire)
  {
    using nes::net::receive_at_least;
    return receive_at_least(*this, at_least_size, time_expire);
  }

  template <class R, class P>
  void tls_socket::receive_remaining(vector<std::byte>& data, size_t total_size, duration<R, P> time_expire)
  {
    using nes::net::receive_remaining;
    receive_remaining(*this, data, total_size, time_expire);
  }

  void same_thread_handshake(tls_socket& a, tls_socket& b)
  {
    // If connecting the server client and client in same thread need make manual TLS handshake
    tls_socket* p_serv { nullptr };
    tls_socket* p_cli { nullptr };
    if (a.m_handshake == tls_socket::handshake_state::connect &&
        b.m_handshake == tls_socket::handshake_state::accept)
    {
      p_serv = &b; p_cli = &a;
    }
    else if (a.m_handshake == tls_socket::handshake_state::accept &&
             b.m_handshake == tls_socket::handshake_state::connect)
    {
      p_serv = &a; p_cli = &b;
    }
    else
      throw nes_exc { "The state of sockets are incompatiple to perform the handshake." };

    // Make the handshake
    tls_socket* p_sock = p_serv;
    int ret = -1;
    do {
      switch (p_sock->m_handshake)
      {
        case tls_socket::handshake_state::connect:
          ret = SSL_connect(p_sock->m_sock_ssl);
          break;

        case tls_socket::handshake_state::accept:
          ret = SSL_accept(p_sock->m_sock_ssl);
          break;

        case tls_socket::handshake_state::ok:
          throw nes_exc { "Handshake already over." };
      }

      if (ret == 0)
        throw nes_exc { "Handshake error." };
      else if (ret == -1)
      {
        auto errcode = SSL_get_error(p_sock->m_sock_ssl, -1);
        switch(errcode)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
          {
            // Troca-se os atores
            if (p_sock == p_serv)
              p_sock = p_cli;
            else
              p_sock = p_serv;
            break;
          }
          default:
            throw nes_exc { "Error making the handshake. Cod.: " + to_string(errcode) };
        }
      }
    } while (ret != 1);

    p_cli->m_handshake = tls_socket::handshake_state::ok;
    p_serv->m_handshake = tls_socket::handshake_state::ok;
  }

  void initialize_OpenSSL()
  {
    // OpenSSL Init
    SSL_load_error_strings();
    SSL_library_init();
  }

  static atomic<unsigned> openssl_ctx_contador { 0 };
  static SSL_CTX *openssl_ctxe { nullptr };

  SSL_CTX *openssl_ctx()
  {
    if (openssl_ctx_contador++ == 0)
    {
      openssl_ctxe = SSL_CTX_new(TLS_method());
      if (!openssl_ctxe)
      {
        --openssl_ctx_contador;
        throw nes_exc { "Fail to alocate global OpenSSL context." };
      }
    }

    return openssl_ctxe;
  }

  void openssl_ctx_free()
  {
    if (!--openssl_ctx_contador)
      SSL_CTX_free(openssl_ctxe);
  }

}
