#ifndef NES_SO__WIN_SOCKET_H
#define NES_SO__WIN_SOCKET_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

// Forward declare so do not need include 'windows.h'
using UINT_PTR = std::conditional_t<sizeof(void*) == 4, std::uint32_t, std::uint64_t>;
using SOCKET = UINT_PTR;

namespace nes::so {

  class win_socket final
  {
    // WinSock2 handle
    SOCKET m_winsocket;

    // IPv4 Data
    std::string m_ipv4_address { "0.0.0.0" };
    unsigned m_ipv4_port { 0 };

  public:
    win_socket();
    ~win_socket();
    win_socket(win_socket&&) noexcept;
    win_socket& operator=(win_socket&&) noexcept;

    // No copy (unique sock handle)
    win_socket(const win_socket&) = delete;
    win_socket& operator=(const win_socket&) = delete;

    // Access
    const std::string& ipv4_address() const;
    unsigned ipv4_port() const;

    using native_handle_type = SOCKET;
    native_handle_type native_handle() const;

    // Server API
    // Put the sock on non-block listening (Ipv4 Port)
    void listen(unsigned);

    // Server Socket status
    bool is_listening() const;
    bool has_client();

    std::optional<win_socket> accept();

    // Client API
    // Connection
    void connect(std::string, unsigned);
    void disconnect();
    bool is_connected() const;

    // I/O
    void send(std::span<const std::byte>);
    std::vector<std::byte> receive();
  };

}

#endif
// NES_SO__WIN_SOCKET_H
