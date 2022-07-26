#ifndef NES_NET__TLS_SOCKET_H
#define NES_NET__TLS_SOCKET_H

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "socket.h"

struct ssl_st;
using SSL = struct ssl_st;

namespace nes::net {

  class tls_socket final
  {
    // Socket e a conexão OpenSSL
    socket m_sock;
    SSL *m_sock_ssl { nullptr };

    // Handshake estado e função
    enum class estado_handshake { conectar, aceitar, ok };
    estado_handshake m_handshake { estado_handshake::conectar };
    void handshake();

  public:
    // Construtor
    tls_socket();
    tls_socket(SSL*, socket);
    tls_socket(std::string, unsigned);

    ~tls_socket();

    tls_socket(const tls_socket&) = delete;
    tls_socket(tls_socket&&);

    const tls_socket& operator=(const tls_socket&) const = delete;
    tls_socket& operator=(tls_socket&&);

    // Conexão no endereço especificado
    void conectar(std::string, unsigned);
    void desconectar();

    // Extenções do TLS
    // Para casos de servidores que hospedam vários sites, informa o virtual host
    void tls_ext_host_name(std::string);

    // Acesso a informações
    const std::string& end_ipv4() const;
    unsigned porta_ipv4() const;
    bool conectado() const;

    std::string cifra() const;
    std::string protocolo_tls() const;

    // Funções de E/S
    void enviar(std::span<const std::byte>);
    void enviar(std::string_view);
    std::vector<std::byte> receber();

    // Utilitários de E/S
    template <class R, class P = std::ratio<1>>
    std::pair<std::vector<std::byte>, std::ptrdiff_t>
    receber_ate_delim(std::span<const std::byte>, std::chrono::duration<R, P>, std::size_t);

    template <class R, class P = std::ratio<1>>
    std::vector<std::byte> receber_ate_tam(std::size_t, std::chrono::duration<R, P>);

    template <class R, class P = std::ratio<1>>
    std::vector<std::byte> receber_ao_menos(std::size_t, std::chrono::duration<R, P>);

    template <class R, class P>
    void receber_resto(std::vector<std::byte>&, size_t, std::chrono::duration<R, P>);

    // Utilitário para realizar a conexão em uma única thread
    friend void estabelecer_handshake(tls_socket&, tls_socket&);
  };

}

#endif
// NES_NET__TLS_SOCKET_H
