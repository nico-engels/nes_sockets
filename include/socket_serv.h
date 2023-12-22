#ifndef NES_NET__SOCKET_SERV_H
#define NES_NET__SOCKET_SERV_H

#include <optional>
#include "socket.h"
#include "win_socket.h"
#include "unix_socket.h"

namespace nes::net {

  template <class S>
  class socket_serv_tmpl final
  {
    // Socket da API Nativa
    S m_sock_so;

  public:
    socket_serv_tmpl() = default;
    explicit socket_serv_tmpl(unsigned);

    // Iniciar a escuta
    void escutar(unsigned);

    // Acesso
    unsigned porta_ipv4() const;

    // Função de Status do Socket
    bool escutando() const;
    bool ha_cliente();

    // Aceitar conexão
    std::optional<socket_tmpl<S>> aceitar();
  };

  using socket_serv = socket_serv_tmpl<socket_so_impl>;
}

#endif
// NES_NET__SOCKET_SERV_H