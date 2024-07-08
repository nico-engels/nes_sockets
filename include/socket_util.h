#ifndef NES_NET__SOCKET_UTIL_H
#define NES_NET__SOCKET_UTIL_H

#include <chrono>
#include <cstddef>
#include <span>
#include <vector>

namespace nes::net {

  // Function returns [wait_io_step_min, wait_io_step_max] bassed in parameter tries
  // Where 0 == tries == wait_io_step_min and tries == io_max_retry == wait_io_step_max
  std::chrono::milliseconds calculate_interval_retry(std::size_t);

  // Input Commom algorithms normal and secure socket
  template <class S, class R, class P = std::ratio<1>>
  std::pair<std::vector<std::byte>, std::size_t>
  receive_until_delimiter(S&, std::span<const std::byte>, std::chrono::duration<R, P>, std::size_t);

  template <class S, class R, class P = std::ratio<1>>
  std::vector<std::byte> receive_until_size(S&, std::size_t, std::chrono::duration<R, P>);

  template <class S, class R, class P = std::ratio<1>>
  std::vector<std::byte> receive_at_least(S&, std::size_t, std::chrono::duration<R, P>);

  template <class S, class R, class P>
  void receive_remaining(S&, std::vector<std::byte>&, size_t, std::chrono::duration<R, P>);

}

#endif
// NES_NET__SOCKET_UTIL_H