#ifndef NES_NET__SOCKET_SERV_H
#define NES_NET__SOCKET_SERV_H

#include <optional>
#include "socket.h"

namespace nes::net {

  template <class S>
  class socket_serv_tmpl final
  {
    // SO Native Socket
    S m_sock_so;

  public:
    socket_serv_tmpl() = default;

    // Constructor (Port)
    explicit socket_serv_tmpl(unsigned);

    // Put the sock on non-block listening (Ipv4 Port)
    void listen(unsigned);

    unsigned ipv4_port() const;

    bool is_listening() const;
    bool has_client();

    std::optional<socket> accept();
  };

  using socket_serv = socket_serv_tmpl<socket::os_socket_type>;
}

#endif
// NES_NET__SOCKET_SERV_H
