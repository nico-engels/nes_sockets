#include "tls_socket_serv.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
using namespace std;
using namespace std::chrono_literals;

namespace nes::net {

   // Controle inicialização/finalização da biblioteca (definido no tls_socket)
  extern once_flag inicializa_lib;
  void inicializa_OpenSSL();

  // Inicialização/Finalização do contexto da biblioteca
  SSL_CTX *openssl_ctx();
  void openssl_ctx_free();

  // Classes auxiliares
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
    bool m_iniciou { false };
  public:
    // Inicialização única do contexto
    ctxssl_ini() { openssl_ctx(); }

    // Executa no construtor, se deu algum problema decrementa o contador e desaloca o contexto
    ~ctxssl_ini() { if (!m_iniciou) openssl_ctx_free(); }

    void iniciou() { m_iniciou = true; };
  };
  }

  // tls_socket_serv
  tls_socket_serv::tls_socket_serv()
  {
    // Inicialização da lib
    // Apenas uma vez na execução do programa
    call_once(inicializa_lib, inicializa_OpenSSL);

    openssl_ctx();
  }

  tls_socket_serv::tls_socket_serv(unsigned ipv4_porta, string caminho_chave_pub, string caminho_chave_priv)
  {
    // Inicialização da lib
    // Apenas uma vez na execução do programa
    call_once(inicializa_lib, inicializa_OpenSSL);

    // Inicialização única do contexto
    ctxssl_ini ini;

    this->escutar(ipv4_porta, move(caminho_chave_pub), move(caminho_chave_priv));

    // OK
    ini.iniciou();
  }

  tls_socket_serv::tls_socket_serv(tls_socket_serv&& outro)
    : m_sock { move(outro.m_sock) }
    , m_caminho_chave_pub { move(outro.m_caminho_chave_pub) }
    , m_caminho_chave_priv { move(outro.m_caminho_chave_priv) }
  {
    // Contexto
    openssl_ctx();
  }

  tls_socket_serv& tls_socket_serv::operator=(tls_socket_serv&& outro)
  {
    swap(m_sock, outro.m_sock);
    swap(m_caminho_chave_pub, outro.m_caminho_chave_pub);
    swap(m_caminho_chave_priv, outro.m_caminho_chave_priv);

    return *this;
  }

  tls_socket_serv::~tls_socket_serv()
  {
    openssl_ctx_free();
  }

  void tls_socket_serv::escutar(unsigned ipv4_porta, string caminho_chave_pub, string caminho_chave_priv)
  {
    // Configura os certificados do socket TLS
    if(SSL_CTX_use_certificate_file(openssl_ctx(), caminho_chave_pub.c_str(), SSL_FILETYPE_PEM) <= 0)
      throw runtime_error { "Erro ao configurar a chave pública!" };

    if(SSL_CTX_use_PrivateKey_file(openssl_ctx(), caminho_chave_priv.c_str(), SSL_FILETYPE_PEM) <= 0)
      throw runtime_error { "Erro ao configurar a chave privada!" };

    if (!SSL_CTX_check_private_key(openssl_ctx()))
      throw runtime_error { "Divergência chave pública/privada!" };

    // Seta internamente
    m_sock.escutar(ipv4_porta);
    m_caminho_chave_pub = move(caminho_chave_pub);
    m_caminho_chave_priv = move(caminho_chave_priv);
  }

  unsigned tls_socket_serv::porta_ipv4() const
  {
    return m_sock.porta_ipv4();
  }

  const string& tls_socket_serv::caminho_chave_pub() const
  {
    return m_caminho_chave_pub;
  }

  const string& tls_socket_serv::caminho_chave_priv() const
  {
    return m_caminho_chave_priv;
  }

  bool tls_socket_serv::escutando() const
  {
    return m_sock.escutando();
  }

  bool tls_socket_serv::ha_cliente()
  {
    return m_sock.ha_cliente();
  }

  optional<tls_socket> tls_socket_serv::aceitar()
  {
    // Primeiro recebe o cliente do socket
    auto c = m_sock.aceitar();
    if (!c)
      return nullopt;

    SSL *sock_ssl = SSL_new(openssl_ctx());
    if (!sock_ssl)
      throw runtime_error { "Não foi possível alocar a conexão OpenSSL para o cliente!" };
    sockssl_rai csock_ssl(sock_ssl);

    int ret = SSL_set_fd(csock_ssl.handle(), static_cast<int>(c->native_handle()));
    if (ret != 1)
      throw runtime_error { "Não foi possível configurar a conexão OpenSSL com o socket nativo!" };

    // Ok
    return tls_socket { csock_ssl.release(), move(*c) };
  }

}