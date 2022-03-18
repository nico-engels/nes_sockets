#ifndef NES_SO__UNIX_SOCKET_H
#define NES_SO__UNIX_SOCKET_H

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace nes::so {

  class unix_socket final
  {
    // Manipulador do Socket Unix
    int m_unix_sd;

    // Informações de endereço IPv4
    std::string m_end_ipv4 { "0.0.0.0" };
    unsigned m_porta_ipv4 { 0 };

  public:
    unix_socket();
    ~unix_socket();

    // Movimento
    unix_socket(unix_socket&&) noexcept;
    unix_socket& operator=(unix_socket&&) noexcept;

    // Sem cópias
    unix_socket(const unix_socket&) = delete;
    unix_socket& operator=(const unix_socket&) = delete;

    // Acesso
    const std::string& end_ipv4() const;
    unsigned porta_ipv4() const;

    using native_handle_type = int;
    native_handle_type native_handle() const;

    // Servidor
    // Começa a ouvir
    void escutar(unsigned);

    // Funções de Status do Socket
    bool conectado() const;
    bool escutando() const;
    bool ha_cliente();

    // Aceita a conexão
    std::optional<unix_socket> aceitar();

    // Cliente - Conecta no endereço IPv4
    void conectar(std::string, unsigned);
    void desconectar();

    // E/S
    void enviar(std::span<const std::byte>);
    std::vector<std::byte> receber();
  };

}

#endif
// NES_SO__UNIX_SOCKET_H
