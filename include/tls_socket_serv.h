#ifndef NES_NET__TLS_SOCKET_SERV_H
#define NES_NET__TLS_SOCKET_SERV_H

#include <optional>
#include "socket_serv.h"
#include "tls_socket.h"

namespace nes::net {

  class tls_socket_serv final
  {
    // SO Native Socket Server
    socket_serv m_sock;

    // Public/Private Key Pair Path
    std::string m_pubkey_path;
    std::string m_privkey_path;

  public:
    tls_socket_serv();
    tls_socket_serv(unsigned, std::string, std::string);

    tls_socket_serv(tls_socket_serv&&);
    tls_socket_serv& operator=(tls_socket_serv&&);

    tls_socket_serv(const tls_socket_serv&) = delete;
    const tls_socket_serv& operator=(const tls_socket_serv&) = delete;

    ~tls_socket_serv();

    // Put the sock on non-block listening (Ipv4 Port, Public Key Path, Private Key Path)
    void listen(unsigned, std::string, std::string);

    unsigned ipv4_port() const;

    const std::string& public_key_path() const;
    const std::string& private_key_path() const;

    bool is_listening() const;
    bool has_client();

    std::optional<tls_socket> accept();
  };
}

#endif
// NES_NET__TLS_SOCKET_SERV_H