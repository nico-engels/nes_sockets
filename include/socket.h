#ifndef NES_NET__SOCKET_H
#define NES_NET__SOCKET_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "cfg.h"
#include "unix_socket.h"
#include "win_socket.h"

namespace nes::net {

  template <class S>
  class socket_tmpl final
  {
    // Socket da API Nativa
    S m_sock_so;

  public:
    // Construtor
    socket_tmpl() = default;
    socket_tmpl(S);
    socket_tmpl(std::string, unsigned);

    // Conexão no endereço especificado
    void conectar(std::string, unsigned);
    void desconectar();

    // Acesso os dados sobre o socket
    const std::string& end_ipv4() const;
    unsigned porta_ipv4() const;
    bool conectado() const;

    // Funções relativas ao socket nativo
    using native_handle_type = S::native_handle_type;
    native_handle_type native_handle() const;

    // Funções de E/S
    void enviar(std::span<const std::byte>);
    void enviar(std::string_view);
    std::vector<std::byte> receber();

    // Utilitários de E/S
    template <class R, class P = std::ratio<1>>
    std::pair<std::vector<std::byte>, std::ptrdiff_t>
    receber_ate_delim(std::span<const std::byte>, std::chrono::duration<R, P>, std::size_t);

    template <class R, class P = std::ratio<1>>
    std::vector<std::byte> receber_ate_tam(std::size_t, std::chrono::duration<R, P>);

    template <class R, class P = std::ratio<1>>
    std::vector<std::byte> receber_ao_menos(std::size_t, std::chrono::duration<R, P>);

    template <class R, class P>
    void receber_resto(std::vector<std::byte>&, size_t, std::chrono::duration<R, P>);
  };

  using socket_so_impl = std::conditional_t<nes::cfg::so::is_windows, nes::so::win_socket
                                                                    , nes::so::unix_socket>;
  using socket = socket_tmpl<socket_so_impl>;

}

#endif
// NES_NET__SOCKET_H