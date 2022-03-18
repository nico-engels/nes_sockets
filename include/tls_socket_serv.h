#ifndef NES_NET__TLS_SOCKET_SERV_H
#define NES_NET__TLS_SOCKET_SERV_H

#include <optional>
#include "socket_serv.h"
#include "tls_socket.h"

namespace nes::net {

  class tls_socket_serv final
  {
    // Socket de escuta
    socket_serv m_sock;

    // Caminho das chaves pública e privada
    std::string m_caminho_chave_pub;
    std::string m_caminho_chave_priv;

  public:
    tls_socket_serv();
    tls_socket_serv(unsigned, std::string, std::string);

    tls_socket_serv(tls_socket_serv&&);
    tls_socket_serv& operator=(tls_socket_serv&&);

    // Sem cópias
    tls_socket_serv(const tls_socket_serv&) = delete;
    const tls_socket_serv& operator=(const tls_socket_serv&) = delete;

    ~tls_socket_serv();

    // Iniciar a escuta
    void escutar(unsigned, std::string, std::string);

    // Acesso
    unsigned porta_ipv4() const;

    const std::string& caminho_chave_pub() const;
    const std::string& caminho_chave_priv() const;

    // Função de Status do Socket
    bool escutando() const;
    bool ha_cliente();

    // Aceitar conexão
    std::optional<tls_socket> aceitar();
  };

}

#endif
// NES_NET__TLS_SOCKET_SERV_H