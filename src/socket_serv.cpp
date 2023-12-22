#include "socket_serv.h"

using namespace std;

namespace nes::net {

  template <class S>
  socket_serv_tmpl<S>::socket_serv_tmpl(unsigned ipv4_porta)
  {
    m_sock_so.escutar(ipv4_porta);
  }

  template <class S>
  void socket_serv_tmpl<S>::escutar(unsigned ipv4_porta)
  {
    m_sock_so.escutar(ipv4_porta);
  }

  template <class S>
  unsigned socket_serv_tmpl<S>::porta_ipv4() const
  {
    return m_sock_so.porta_ipv4();
  }

  template <class S>
  bool socket_serv_tmpl<S>::escutando() const
  {
    return m_sock_so.escutando();
  }

  template <class S>
  bool socket_serv_tmpl<S>::ha_cliente()
  {
    return m_sock_so.ha_cliente();
  }

  template <class S>
  optional<socket_tmpl<S>> socket_serv_tmpl<S>::aceitar()
  {
    auto sock_act = m_sock_so.aceitar();
    if (sock_act)
      return socket_tmpl<S> { move(*sock_act) };
    else
      return nullopt;
  }

  // Instanciar Template
  template class socket_serv_tmpl<socket_so_impl>;
}
