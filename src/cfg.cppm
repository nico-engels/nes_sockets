export module cfg;

import std;

export namespace nes::cfg {

  namespace net {
     // Waiting times
     constexpr auto wait_io_step_min = std::chrono::milliseconds {  25 };
     constexpr auto wait_io_step_max = std::chrono::milliseconds { 250 };

     // Packet Size
     constexpr auto packet_size = std::size_t { 8'192 };

     // Retries
     constexpr auto io_max_retry = std::size_t { 100 };
  }

  namespace so {
    #ifdef _WIN32
    constexpr auto is_windows = true;
    #else
    constexpr auto is_windows = false;
    #endif
  }

}