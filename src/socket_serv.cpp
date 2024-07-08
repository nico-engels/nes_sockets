#include "socket_serv.h"

using namespace std;

namespace nes::net {

  template <class S>
  socket_serv_tmpl<S>::socket_serv_tmpl(unsigned port)
  {
    m_sock_so.listen(port);
  }

  template <class S>
  void socket_serv_tmpl<S>::listen(unsigned port)
  {
    m_sock_so.listen(port);
  }

  template <class S>
  unsigned socket_serv_tmpl<S>::ipv4_port() const
  {
    return m_sock_so.ipv4_port();
  }

  template <class S>
  bool socket_serv_tmpl<S>::is_listening() const
  {
    return m_sock_so.is_listening();
  }

  template <class S>
  bool socket_serv_tmpl<S>::has_client()
  {
    return m_sock_so.has_client();
  }

  template <class S>
  optional<socket> socket_serv_tmpl<S>::accept()
  {
    auto sock_act = m_sock_so.accept();
    if (sock_act)
      return socket { move(*sock_act) };
    else
      return nullopt;
  }

  template class socket_serv_tmpl<socket_so_impl>;
}
