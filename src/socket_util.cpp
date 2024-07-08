#include "socket_util.h"

#include <algorithm>
#include <thread>
#include "cfg.h"
#include "net_exc.h"
#include "socket.h"
#include "tls_socket.h"
using namespace std;
using namespace std::chrono;
namespace rng = std::ranges;
using namespace nes;

namespace nes::net {

  milliseconds calculate_interval_retry(size_t retry_count)
  {
    // The time span grows as the more tries
    // Be 0 == wait_io_step_min and io_max_retry == wait_io_step_max
    retry_count = min(retry_count, cfg::net::io_max_retry);
    double interval_dif = static_cast<double>((cfg::net::wait_io_step_max - cfg::net::wait_io_step_min).count());
    double retry_coef = static_cast<double>(retry_count) / cfg::net::io_max_retry;
    return cfg::net::wait_io_step_min + milliseconds { static_cast<milliseconds::rep>(interval_dif * retry_coef) };
  }

  // Input Algorithms
  template <class S, class R, class P>
  pair<vector<byte>, size_t> receive_until_delimiter(S& sock, span<const byte> delim, duration<R, P> time_expire, size_t max_size)
  {
    vector<byte> ret;

    // Read data until receive the deliminator
    // The time e size params sets the threasholds for the reading
    auto start = steady_clock::now();
    milliseconds interval = cfg::net::wait_io_step_min;
    size_t retry_count = 0;
    while (steady_clock::now() - start < time_expire)
    {
      const auto data = sock.receive();
      if (data.size() > 0)
      {
        // Excess data check
        if (ret.size() + data.size() > max_size)
          throw socket_excess_data { "Received more data ({}B) than the maximum ({}B)!", ret.size() + data.size(), max_size };

        // Collect in the return value and try to find the deliminator
        ret.insert(ret.end(), data.begin(), data.end());
        if (auto it = rng::search(ret, delim).begin(); it != ret.end())
          return { ret, static_cast<size_t>(it - ret.begin()) };

        // When read some data reset the progressive waiting
        interval = cfg::net::wait_io_step_min;
        retry_count = 0;
      }
      else
      {
        // If no data receive sleeps (at every retry a little more time)
        interval = calculate_interval_retry(++retry_count);
        this_thread::sleep_for(interval);
      }
    }

    throw socket_timeout { "Wait time ({}) expired while especting the deliminator! Received {} bytes!", time_expire, ret.size() };
  }

  template <class S, class R, class P>
  vector<byte> receive_until_size(S& sock, size_t total_size, duration<R, P> time_expire)
  {
    vector<byte> ret;

    // Read data until receive the exact number of bytes
    auto start = steady_clock::now();
    milliseconds interval = cfg::net::wait_io_step_min;
    size_t retry_count = 0;
    while (steady_clock::now() - start < time_expire)
    {
      const auto data = sock.receive();
      if (data.size() > 0)
      {
        // Excess data check
        if (ret.size() + data.size() > total_size)
          throw socket_excess_data { "Received more data ({}B) than the expected ({}B)!",
                                     ret.size() + data.size(), total_size };

        // Collect in the return value and check if received all the data needed
        ret.insert(ret.end(), data.begin(), data.end());
        if (ret.size() == total_size)
          return ret;

        // When read some data reset the progressive waiting
        interval = cfg::net::wait_io_step_min;
        retry_count = 0;
      }
      else
      {
        // If no data receive sleeps (at every retry a little more time)
        interval = calculate_interval_retry(++retry_count);
        this_thread::sleep_for(interval);
      }
    }

    throw socket_timeout { "Wait time {} expired, while expecting {} bytes! Received {} bytes!",
                           time_expire, total_size, ret.size() };
  }

  template <class S, class R, class P>
  vector<byte> receive_at_least(S& sock, size_t at_least_size, duration<R, P> time_expire)
  {
    vector<byte> ret;

    // Read data until receive the al least some number of bytes
    auto start = steady_clock::now();
    milliseconds interval = cfg::net::wait_io_step_min;
    size_t retry_count = 0;
    while (steady_clock::now() - start < time_expire)
    {
      const auto data = sock.receive();
      if (data.size() > 0)
      {
        // Collect in the return value and check if received at least the data needed
        ret.insert(ret.end(), data.begin(), data.end());
        if (ret.size() >= at_least_size)
          return ret;

        // When read some data reset the progressive waiting
        interval = cfg::net::wait_io_step_min;
        retry_count = 0;
      }
      else
      {
        // If no data receive sleeps (at every retry a little more time)
        interval = calculate_interval_retry(++retry_count);
        this_thread::sleep_for(interval);
      }
    }

    throw socket_timeout { "Waiting time {} expired, while expecting at least {} bytes! Received {} bytes!",
                           time_expire, at_least_size, ret.size() };
  }

  template <class S, class R, class P>
  void receive_remaining(S& sock, vector<byte>& data, size_t total_size, duration<R, P> time_expire)
  {
    if (data.size() < total_size)
    {
      auto rem = receive_until_size(sock, total_size - data.size(), time_expire);
      data.insert(data.end(), rem.begin(), rem.end());
    }
  }

  // Template instantiations (at end to work with gcc and clang)
  template pair<vector<byte>, size_t> receive_until_delimiter(socket&, span<const byte>, seconds, size_t);
  template pair<vector<byte>, size_t> receive_until_delimiter(socket&, span<const byte>, milliseconds, size_t);
  template vector<byte> receive_until_size(socket&, size_t, seconds);
  template vector<byte> receive_until_size(socket&, size_t, milliseconds);
  template vector<byte> receive_at_least(socket&, size_t, seconds);
  template void receive_remaining(socket&, vector<byte>&, size_t, seconds);

  template pair<vector<byte>, size_t> receive_until_delimiter(tls_socket&, span<const byte>, seconds, size_t);
  template pair<vector<byte>, size_t> receive_until_delimiter(tls_socket&, span<const byte>, milliseconds, size_t);
  template vector<byte> receive_until_size(tls_socket&, size_t, seconds);
  template vector<byte> receive_until_size(tls_socket&, size_t, milliseconds);
  template vector<byte> receive_at_least(tls_socket&, size_t, seconds);
  template void receive_remaining(tls_socket&, vector<byte>&, size_t, seconds);
}
