export module socket;

import <chrono>;
import <span>;
import <string>;
import <string_view>;
import <type_traits>;
import <vector>;
import cfg;
import socket_util;
import unix_socket;
import win_socket;

// Declaração
export namespace nes::net {

  template <class S>
  class socket_tmpl final
  {
    // SO Native Socket
    S m_sock_so;

  public:
    // Exposition
    using os_socket_type = S;

    socket_tmpl() = default;
    socket_tmpl(const S&) = delete;
    socket_tmpl(S&&);

    // Constructor (Host, Port)
    socket_tmpl(std::string, unsigned);

    // Connection (Host, Port)
    void connect(std::string, unsigned);
    void disconnect();

    // IPv4 Connection Data
    const std::string& ipv4_address() const;
    unsigned ipv4_port() const;
    bool is_connected() const;

    // Native handle
    using native_handle_type = S::native_handle_type;
    native_handle_type native_handle() const;

    // I/O basic functions (binary or binary char)
    void send(std::span<const std::byte>);
    void send(std::string_view);
    [[nodiscard]] std::vector<std::byte> receive();

    // I/O basic utilities
    // Where exists the time_expire and/or max_size are used as maximum threasholds
    // Spin receiving data until finds the delim arg, return the data and pos of delim in data
    template <class R, class P = std::ratio<1>>
    [[nodiscard]] std::pair<std::vector<std::byte>, std::size_t>
    receive_until_delimiter(std::span<const std::byte> delim, std::chrono::duration<R, P> time_expire,
      std::size_t max_size);

    // Read data until receive the exact_size number of bytes
    template <class R, class P = std::ratio<1>>
    [[nodiscard]] std::vector<std::byte> receive_until_size(std::size_t exact_size,
      std::chrono::duration<R, P> time_expire);

    // Read data until receive the >= at_least_size bytes
    template <class R, class P = std::ratio<1>>
    [[nodiscard]] std::vector<std::byte> receive_at_least(std::size_t at_least_size,
      std::chrono::duration<R, P> time_expire);

    // Complete the data arg until the data.size() is equals arg total_size
    template <class R, class P>
    void receive_remaining(std::vector<std::byte>& data, size_t total_size, std::chrono::duration<R, P> time_expire);
  };

  using socket_so_impl = std::conditional_t<nes::cfg::so::is_windows, nes::so::win_socket
                                                                    , nes::so::unix_socket>;
  using socket = socket_tmpl<socket_so_impl>;
}

// Implementação
using namespace std;
using namespace std::chrono;

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

}
