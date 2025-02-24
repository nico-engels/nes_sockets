#ifndef NES_NET__TLS_SOCKET_H
#define NES_NET__TLS_SOCKET_H

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "socket.h"

struct ssl_st;
using SSL = struct ssl_st;

namespace nes::net {

  class tls_socket final
  {
    // Socket and OpenSSL handler
    socket m_sock;
    SSL *m_sock_ssl { nullptr };

    // TLS Handshake state
    enum class handshake_state { connect, accept, ok };
    handshake_state m_handshake { handshake_state::connect };
    void handshake();

  public:
    // Constructor 
    tls_socket();
    tls_socket(SSL*, socket);

    // (Host, port)
    tls_socket(std::string, unsigned);

    ~tls_socket();

    tls_socket(const tls_socket&) = delete;
    tls_socket(tls_socket&&);

    const tls_socket& operator=(const tls_socket&) const = delete;
    tls_socket& operator=(tls_socket&&);

    // Connection (Host, port)
    void connect(std::string, unsigned);
    void disconnect();

    // TLS Extensions
    // Virtual host (same host many sites)
    void tls_ext_host_name(std::string);

    // IPv4 Connection Data
    const std::string& ipv4_address() const;
    unsigned ipv4_port() const;
    bool is_connected() const;

    std::string cipher() const;
    std::string tls_protocol() const;

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

    // Make manual TLS handshake (auxiliary function same thread connection)
    friend void same_thread_handshake(tls_socket&, tls_socket&);
  };

}

#endif
// NES_NET__TLS_SOCKET_H
