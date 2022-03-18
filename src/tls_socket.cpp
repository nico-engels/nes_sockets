#include "tls_socket.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "socket_util.h"
#include "cfg.h"
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace nes::net {

  // Controle inicialização/finalização da biblioteca
  once_flag inicializa_lib;
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

  tls_socket::tls_socket()
  {
    // Inicialização da lib
    // Apenas uma vez na execução do programa
    call_once(inicializa_lib, inicializa_OpenSSL);

    // Contexto
    openssl_ctx();
  }

  tls_socket::tls_socket(SSL* ssl, socket s)
    : m_sock { move(s) }
    , m_sock_ssl { ssl }
    , m_handshake { estado_handshake::aceitar }
  {
    // Contexto
    openssl_ctx();
  }

  tls_socket::tls_socket(string ip, unsigned porta)
    : m_sock { ip, porta }
  {
    // Inicialização da lib
    // Apenas uma vez na execução do programa
    call_once(inicializa_lib, inicializa_OpenSSL);

    // Contexto
    ctxssl_ini ini;

    // Manipulador da conexão
    SSL *sock_ssl = SSL_new(openssl_ctx());
    if (!sock_ssl)
      throw runtime_error { "Não foi possível alocar a conexão OpenSSL para o cliente!" };
    sockssl_rai ssock_ssl(sock_ssl);

    // Linkar com o socket nativo
    int ret = SSL_set_fd(ssock_ssl.handle(), static_cast<int>(m_sock.native_handle()));
    if (ret != 1)
      throw runtime_error { "Não foi possível configurar a conexão OpenSSL com o socket nativo!" };

    // OK
    ini.iniciou();
    m_sock_ssl = ssock_ssl.release();
  }

  tls_socket::tls_socket(tls_socket&& outro)
    : m_sock { move(outro.m_sock) }
    , m_sock_ssl { outro.m_sock_ssl }
    , m_handshake { outro.m_handshake }
  {
    // Contexto
    openssl_ctx();

    outro.m_sock_ssl = nullptr;
  }

  tls_socket& tls_socket::operator=(tls_socket&& outro)
  {
    swap(m_sock, outro.m_sock);
    swap(m_sock_ssl, outro.m_sock_ssl);
    swap(m_handshake, outro.m_handshake);

    return *this;
  }

  tls_socket::~tls_socket()
  {
    // Manipulador da conexão
    if (m_sock_ssl)
      SSL_free(m_sock_ssl);

    // Contexto
    openssl_ctx_free();
  }

  void tls_socket::handshake()
  {
    constexpr milliseconds intervalo_max = 1000ms;
    constexpr milliseconds intervalo_passo = 50ms;
    milliseconds intervalo = 0ms;

    // Realizar o Handshake
    int ret = -1;
    do {
      switch (m_handshake)
      {
        case estado_handshake::conectar:
          ret = SSL_connect(m_sock_ssl);
          break;

        case estado_handshake::aceitar:
          ret = SSL_accept(m_sock_ssl);
          break;

        case estado_handshake::ok:
          throw runtime_error { "Handshake já realizado!" };
      }

      if (ret == 0)
        throw runtime_error { "Erro no handshake!" };
      else if (ret == -1)
      {
        auto errcode = SSL_get_error(m_sock_ssl, -1);
        switch(errcode)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            break;
          default:
            throw runtime_error {
              "Erro durante o handshake!\n"
              "Cód.: " + to_string(errcode) + "-" + ERR_error_string(errcode, NULL)
            };
        }
        intervalo += intervalo_passo;
        if (intervalo >= intervalo_max)
          throw runtime_error { "Timeout no envio de dados!" };

        this_thread::sleep_for(intervalo_passo);
      }
    } while (ret != 1);

    m_handshake = estado_handshake::ok;
  }

  const string& tls_socket::end_ipv4() const
  {
    return m_sock.end_ipv4();
  }

  unsigned tls_socket::porta_ipv4() const
  {
    return m_sock.porta_ipv4();
  }

  bool tls_socket::conectado() const
  {
    return m_sock.conectado();
  }

  string tls_socket::cifra() const
  {
    if (m_sock_ssl)
      return string { SSL_get_cipher(m_sock_ssl) };
    else
      return string {};
  }

  void tls_socket::enviar(span<const byte> dados)
  {
    // Validação se está conectado
    if (!m_sock.conectado())
      throw runtime_error { "O socket TLS precisa estar conectado para enviar dados!" };

    if (m_handshake != estado_handshake::ok)
      this->handshake();

    milliseconds tempo = 0ms;
    int ret;
    do {
      ret = SSL_write(m_sock_ssl, dados.data(), static_cast<int>(dados.size()));
      if (ret > 0)
      {
        if (static_cast<size_t>(ret) != dados.size())
          throw runtime_error { "Não conseguiu enviar todos os dados!" };
      }
      else
      {
        auto coderr = SSL_get_error(m_sock_ssl, ret);
        switch(coderr)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            break;
          default:
            throw runtime_error { "Erro no envio dos dados! Cód.: " + to_string(coderr) };
        }
        tempo += cfg::net::intervalo_passo;
        if (tempo >= cfg::net::timeout)
          throw runtime_error { "Timeout no envio de dados!" };

        this_thread::sleep_for(cfg::net::intervalo_passo);
      }
    } while (ret <= 0);
  }

  void tls_socket::enviar(string_view dados_str)
  {
    auto b = reinterpret_cast<const byte*>(&*dados_str.begin());
    auto e = reinterpret_cast<const byte*>(&*dados_str.end());
    this->enviar(span<const byte> { b, e });
  }

  vector<byte> tls_socket::receber()
  {
    // Validação
    if (!m_sock.conectado())
      throw runtime_error { "Socket TLS não conectado para receber dados!" };

    if (m_handshake != estado_handshake::ok)
      this->handshake();

    vector<byte> dados;
    vector<byte> buffer(cfg::net::bloco, byte {});

    milliseconds tempo = 0ms;
    int ret;
    do {
      ret = SSL_read(m_sock_ssl, buffer.data(), static_cast<int>(buffer.size()));
      if (ret > 0)
      {
        buffer.resize(static_cast<size_t>(ret));
        dados.insert(dados.end(), buffer.begin(), buffer.end());
      }
      else
      {
        auto coderr = SSL_get_error(m_sock_ssl, ret);
        switch(coderr)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            break;
          default:
          {
            // Pode ser socket fechado normalmente, lança a exceção dentro do receber()
            if (coderr == SSL_ERROR_SYSCALL && ERR_get_error() == 0)
              m_sock.receber();

            throw runtime_error { "SSL_read:SSL_get_error! Cód.: " + to_string(coderr) };
          }
        }
        tempo += cfg::net::intervalo_passo;
        if (tempo >= cfg::net::timeout)
          throw runtime_error { "Timeout no envio de dados!" };

        this_thread::sleep_for(cfg::net::intervalo_passo);
      }
    } while (ret == cfg::net::bloco);

    return dados;
  }

  template <class R, class P>
  pair<vector<byte>, ptrdiff_t>
  tls_socket::receber_ate_delim(span<const byte> delim, duration<R, P> tempo_exp, size_t tam_max)
  {
    using nes::net::receber_ate_delim;
    return receber_ate_delim(*this, delim, tempo_exp, tam_max);
  }

  template <class R, class P>
  vector<byte> tls_socket::receber_ate_tam(size_t tam, duration<R, P> tempo_exp)
  {
    using nes::net::receber_ate_tam;
    return receber_ate_tam(*this, tam, tempo_exp);
  }

  template <class R, class P>
  vector<byte> tls_socket::receber_ao_menos(size_t tam, duration<R, P> tempo_exp)
  {
    using nes::net::receber_ao_menos;
    return receber_ao_menos(*this, tam, tempo_exp);
  }

  template <class R, class P>
  void tls_socket::receber_resto(vector<byte>& dados, size_t tam, duration<R, P> tempo_exp)
  {
    using nes::net::receber_resto;
    receber_resto(*this, dados, tam, tempo_exp);
  }

  void estabelecer_handshake(tls_socket& a, tls_socket& b)
  {
    // Utiliza ponteiros para dar os papéis no handshake
    tls_socket* p_serv { nullptr };
    tls_socket* p_cli { nullptr };
    if (a.m_handshake == tls_socket::estado_handshake::conectar &&
        b.m_handshake == tls_socket::estado_handshake::aceitar)
    {
      p_serv = &b; p_cli = &a;
    }
    else if (a.m_handshake == tls_socket::estado_handshake::aceitar &&
             b.m_handshake == tls_socket::estado_handshake::conectar)
    {
      p_serv = &a; p_cli = &b;
    }
    else
      throw runtime_error { "Sockets estão em estados não compatíveis para handshake!" };

    constexpr milliseconds intervalo_max = 1000ms;
    constexpr milliseconds intervalo_passo = 50ms;
    milliseconds intervalo = 0ms;

    // Realizar o Handshake
    tls_socket* p_sock = p_serv;
    int ret = -1;
    do {
      switch (p_sock->m_handshake)
      {
        case tls_socket::estado_handshake::conectar:
          ret = SSL_connect(p_sock->m_sock_ssl);
          break;

        case tls_socket::estado_handshake::aceitar:
          ret = SSL_accept(p_sock->m_sock_ssl);
          break;

        case tls_socket::estado_handshake::ok:
          throw runtime_error { "Handshake já realizado!" };
      }

      if (ret == 0)
        throw runtime_error { "Erro no handshake!" };
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
            throw runtime_error { "Erro durante o handshake! Cód.: " + to_string(errcode) };
        }
        intervalo += intervalo_passo;
        if (intervalo >= intervalo_max)
          throw runtime_error { "Timeout no envio de dados!" };

        this_thread::sleep_for(intervalo_passo);
      }
    } while (ret != 1);

    p_cli->m_handshake = tls_socket::estado_handshake::ok;
    p_serv->m_handshake = tls_socket::estado_handshake::ok;
  }

  void inicializa_OpenSSL()
  {
    // Inicializa a biblioteca OpenSSL
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
        throw runtime_error { "Não foi possível alocar o contexto OpenSSL para o cliente!" };
      }
    }

    return openssl_ctxe;
  }

  void openssl_ctx_free()
  {
    if (!--openssl_ctx_contador)
      SSL_CTX_free(openssl_ctxe);
  }

  // Instância as funções utilizadas
  template pair<vector<byte>, ptrdiff_t> tls_socket::receber_ate_delim(span<const byte>, seconds, size_t);
  template pair<vector<byte>, ptrdiff_t> tls_socket::receber_ate_delim(span<const byte>, milliseconds, size_t);
  template vector<byte> tls_socket::receber_ate_tam(size_t, seconds);
  template vector<byte> tls_socket::receber_ate_tam(size_t, milliseconds);
  template vector<byte> tls_socket::receber_ao_menos(size_t, seconds);
  template void tls_socket::receber_resto(vector<byte>&, size_t, seconds);


}