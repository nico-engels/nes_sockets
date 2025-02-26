#include "socket.h"

#include <algorithm>
#include <stdexcept>
#include <thread>
#include "net_exc.h"
#include "socket_util.h"
using namespace std;
using namespace std::chrono;
using namespace nes;
using namespace nes::so;

namespace nes::net {

  template <class S>
  socket_tmpl<S>::socket_tmpl(string addr, unsigned port)
  {
    this->connect(move(addr), port);
  }

  template <class S>
  socket_tmpl<S>::socket_tmpl(S&& other)
    : m_sock_so { move(other) }
  {

  }

  template <class S>
  void socket_tmpl<S>::connect(string addr, unsigned port)
  {
    m_sock_so.connect(move(addr), port);
  }

  template <class S>
  void socket_tmpl<S>::disconnect()
  {
    m_sock_so.disconnect();
  }

  template <class S>
  const string& socket_tmpl<S>::ipv4_address() const
  {
    return m_sock_so.ipv4_address();
  }

  template <class S>
  unsigned socket_tmpl<S>::ipv4_port() const
  {
    return m_sock_so.ipv4_port();
  }

  template <class S>
  bool socket_tmpl<S>::is_connected() const
  {
    return m_sock_so.is_connected();
  }

  template <class S>
  typename socket_tmpl<S>::native_handle_type socket_tmpl<S>::native_handle() const
  {
    return m_sock_so.native_handle();
  }

  template <class S>
  void socket_tmpl<S>::send(span<const byte> data)
  {
    m_sock_so.send(data);
  }

  template <class S>
  void socket_tmpl<S>::send(string_view data_str)
  {
    m_sock_so.send(as_bytes(span { data_str.begin(), data_str.end() }));
  }

  template <class S>
  vector<byte> socket_tmpl<S>::receive()
  {
    return m_sock_so.receive();
  }

  template <class S>
  template <class R, class P>
  pair<vector<byte>, size_t>
  socket_tmpl<S>::receive_until_delimiter(span<const byte> delim, duration<R, P> time_expire, size_t max_size)
  {
    using nes::net::receive_until_delimiter;
    return receive_until_delimiter(*this, delim, time_expire, max_size);
  }

  template <class S>
  template <class R, class P>
  vector<byte> socket_tmpl<S>::receive_until_size(size_t exact_size, duration<R, P> time_expire)
  {
    using nes::net::receive_until_size;
    return receive_until_size(*this, exact_size, time_expire);
  }

  template <class S>
  template <class R, class P>
  vector<byte> socket_tmpl<S>::receive_at_least(size_t at_least_size, duration<R, P> time_expire)
  {
    using nes::net::receive_at_least;
    return receive_at_least(*this, at_least_size, time_expire);
  }

  template <class S>
  template <class R, class P>
  void socket_tmpl<S>::receive_remaining(vector<byte>& data, size_t total_size, duration<R, P> time_expire)
  {
    using nes::net::receive_remaining;
    receive_remaining(*this, data, total_size, time_expire);
  }

  // Template class instanciation
  template class socket_tmpl<socket_so_impl>;

  // Function template instanciation
  template pair<vector<byte>, size_t>
  socket_tmpl<socket_so_impl>::receive_until_delimiter(span<const byte>, seconds, size_t);
  template pair<vector<byte>, size_t>
  socket_tmpl<socket_so_impl>::receive_until_delimiter(span<const byte>, milliseconds, size_t);
  template pair<vector<byte>, size_t> 
  socket_tmpl<socket_so_impl>::receive_until_delimiter(span<const byte>, duration<double>, size_t);
  template vector<byte> socket_tmpl<socket_so_impl>::receive_until_size(size_t, seconds);
  template vector<byte> socket_tmpl<socket_so_impl>::receive_until_size(size_t, milliseconds);
  template vector<byte> socket_tmpl<socket_so_impl>::receive_at_least(size_t, seconds);
  template void socket_tmpl<socket_so_impl>::receive_remaining(vector<byte>&, size_t, seconds);
}
