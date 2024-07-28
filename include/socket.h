#ifndef NES_NET__SOCKET_H
#define NES_NET__SOCKET_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "cfg.h"
#include "unix_socket.h"
#include "win_socket.h"

namespace nes::net {

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

#endif
// NES_NET__SOCKET_H