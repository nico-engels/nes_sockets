export module unix_socket;

import <cstddef>;
import <optional>;
import <span>;
import <string>;
import <vector>;

export namespace nes::so {

  class unix_socket final
  {
    // BSD socket handle
    int m_unix_sd;

    // IPv4 Data
    std::string m_ipv4_address { "0.0.0.0" };
    unsigned m_ipv4_port { 0 };

  public:
    unix_socket();
    ~unix_socket();
    unix_socket(unix_socket&&) noexcept;
    unix_socket& operator=(unix_socket&&) noexcept;

    // No copy (unique sock handle)
    unix_socket(const unix_socket&) = delete;
    unix_socket& operator=(const unix_socket&) = delete;

    // Access
    const std::string& ipv4_address() const;
    unsigned ipv4_port() const;

    using native_handle_type = int;
    native_handle_type native_handle() const;

    // Server API
    // Put the sock on non-block listening
    void listen(unsigned);

    // Status
    bool is_listening() const;
    bool has_client();

    std::optional<unix_socket> accept();

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
