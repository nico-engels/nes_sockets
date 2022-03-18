#ifndef NES_NET__SOCKET_UTIL_H
#define NES_NET__SOCKET_UTIL_H

#include <chrono>
#include <cstddef>
#include <span>
#include <vector>

namespace nes::net {

  // Função que varia [intervalo_passo, intervalo_max] baseado nas tentativas
  // Onde 0 tentativas == intervalo_passo e tentativas_max == intervalo_max
  std::chrono::milliseconds calc_intervalo_proporcional(std::size_t);

  // Algoritmos de E/S
  template <class S, class R, class P = std::ratio<1>>
  std::pair<std::vector<std::byte>, std::ptrdiff_t>
  receber_ate_delim(S&, std::span<const std::byte>, std::chrono::duration<R, P>, std::size_t);

  template <class S, class R, class P = std::ratio<1>>
  std::vector<std::byte> receber_ate_tam(S&, std::size_t, std::chrono::duration<R, P>);

  template <class S, class R, class P = std::ratio<1>>
  std::vector<std::byte> receber_ao_menos(S&, std::size_t, std::chrono::duration<R, P>);

  template <class S, class R, class P>
  void receber_resto(S&, std::vector<std::byte>&, size_t, std::chrono::duration<R, P>);

}

#endif
// NES_NET__SOCKET_UTIL_H