#ifndef NES_SO__WIN_SOCKET_H
#define NES_SO__WIN_SOCKET_H

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

// Evitar a importação
using UINT_PTR = std::conditional_t<sizeof(void*) == 4, std::uint32_t, std::uint64_t>;
using SOCKET = UINT_PTR;

namespace nes::so {

  class win_socket final
  {
    // Manipulador do Socket Windows
    SOCKET m_winsocket;

    // Informações de endereço IPv4
    std::string m_end_ipv4 { "0.0.0.0" };
    unsigned m_porta_ipv4 { 0 };

  public:
    win_socket();
    ~win_socket();

    // Movimento
    win_socket(win_socket&&) noexcept;
    win_socket& operator=(win_socket&&) noexcept;

    // Sem cópias
    win_socket(const win_socket&) = delete;
    win_socket& operator=(const win_socket&) = delete;

    // Acesso
    const std::string& end_ipv4() const;
    unsigned porta_ipv4() const;

    using native_handle_type = SOCKET;
    native_handle_type native_handle() const;

    // Servidor
    // Começa a ouvir
    void escutar(unsigned);

    // Funções de Status do Socket
    bool conectado() const;
    bool escutando() const;
    bool ha_cliente();

    // Aceita a conexão
    std::optional<win_socket> aceitar();

    // Cliente - Conecta no endereço IPv4
    void conectar(std::string, unsigned);
    void desconectar();

    // E/S
    void enviar(std::span<const std::byte>);
    std::vector<std::byte> receber();
  };

}

#endif
// NES_SO__WIN_SOCKET_H