module;

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

export module tls_socket_serv;

import <atomic>;
import <chrono>;
import <mutex>;
import <optional>;
import <thread>;
import socket_serv;
import nes_exc;
import tls_socket;

// Declaration
export namespace nes::net {

  class tls_socket_serv final
  {
    // SO Native Socket Server
    socket_serv m_sock;

    // Public/Private Key Pair Path
    std::string m_pubkey_path;
    std::string m_privkey_path;

  public:
    tls_socket_serv();
    tls_socket_serv(unsigned, std::string, std::string);

    tls_socket_serv(tls_socket_serv&&);
    tls_socket_serv& operator=(tls_socket_serv&&);

    tls_socket_serv(const tls_socket_serv&) = delete;
    const tls_socket_serv& operator=(const tls_socket_serv&) = delete;

    ~tls_socket_serv();

    // Put the sock on non-block listening (Ipv4 Port, Public Key Path, Private Key Path)
    void listen(unsigned, std::string, std::string);

    unsigned ipv4_port() const;

    const std::string& public_key_path() const;
    const std::string& private_key_path() const;

    bool is_listening() const;
    bool has_client();

    std::optional<tls_socket> accept();
  };
}

// Implementation
using namespace std;
using namespace std::chrono_literals;
using namespace nes;

namespace nes::net {

  tls_socket_serv::tls_socket_serv()
  {
    call_once(init_lib, initialize_OpenSSL);

    openssl_ctx();
  }

  tls_socket_serv::tls_socket_serv(unsigned port, string pubkey_path, string privkey_path)
  {
    call_once(init_lib, initialize_OpenSSL);

    ctxssl_ini ini;

    this->listen(port, move(pubkey_path), move(privkey_path));

    ini.set_initialized();
  }

  tls_socket_serv::tls_socket_serv(tls_socket_serv&& other)
    : m_sock { move(other.m_sock) }
    , m_pubkey_path { move(other.m_pubkey_path) }
    , m_privkey_path { move(other.m_privkey_path) }
  {
    openssl_ctx();
  }

  tls_socket_serv& tls_socket_serv::operator=(tls_socket_serv&& other)
  {
    swap(m_sock, other.m_sock);
    swap(m_pubkey_path, other.m_pubkey_path);
    swap(m_privkey_path, other.m_privkey_path);

    return *this;
  }

  tls_socket_serv::~tls_socket_serv()
  {
    openssl_ctx_free();
  }

  void tls_socket_serv::listen(unsigned port, string pubkey_path, string privkey_path)
  {
    // TLS Certificates configuration
    if(SSL_CTX_use_certificate_file(openssl_ctx(), pubkey_path.c_str(), SSL_FILETYPE_PEM) <= 0)
      throw nes_exc { "Public Key configuration error." };

    if(SSL_CTX_use_PrivateKey_file(openssl_ctx(), privkey_path.c_str(), SSL_FILETYPE_PEM) <= 0)
      throw nes_exc { "Private Key configuration error." };

    if (!SSL_CTX_check_private_key(openssl_ctx()))
      throw nes_exc { "Public/Private configuration mismatch." };

    m_sock.listen(port);

    // All ok, can set the class
    m_pubkey_path = move(pubkey_path);
    m_privkey_path = move(privkey_path);
  }

  unsigned tls_socket_serv::ipv4_port() const
  {
    return m_sock.ipv4_port();
  }

  const string& tls_socket_serv::public_key_path() const
  {
    return m_pubkey_path;
  }

  const string& tls_socket_serv::private_key_path() const
  {
    return m_privkey_path;
  }

  bool tls_socket_serv::is_listening() const
  {
    return m_sock.is_listening();
  }

  bool tls_socket_serv::has_client()
  {
    return m_sock.has_client();
  }

  optional<tls_socket> tls_socket_serv::accept()
  {
    auto c = m_sock.accept();
    if (!c)
      return nullopt;

    SSL *sock_ssl = SSL_new(openssl_ctx());
    if (!sock_ssl)
      throw nes_exc { "Not possible alocate the OpenSSL client context." };
    sockssl_rai csock_ssl(sock_ssl);

    int ret = SSL_set_fd(csock_ssl.handle(), static_cast<int>(c->native_handle()));
    if (ret != 1)
      throw nes_exc { "Can not bind the SSL handle with native socket." };

    // Ok, adapt the handler and socket to the class
    return tls_socket { csock_ssl.release(), move(*c) };
  }

}