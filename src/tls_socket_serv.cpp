#include "tls_socket_serv.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "nes_exc.h"
using namespace std;
using namespace std::chrono_literals;
using namespace nes;

namespace nes::net {

   // Init/Destroy flag (definied in tls_socket)
  extern once_flag init_lib;
  void initialize_OpenSSL();

  SSL_CTX *openssl_ctx();
  void openssl_ctx_free();

  // Aux RAII temporary handle
  namespace {
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