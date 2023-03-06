#ifndef NES__BYTE_OP_H
#define NES__BYTE_OP_H

#include <array>
#include <bit>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace nes {

  // Conversor para base64 baseado na RFC 4648
  std::string para_base64(std::span<const std::byte>, bool = true, bool = true);
  std::string para_base64(std::string_view, bool = true, bool = true);

  std::vector<std::byte> de_base64(std::string_view, bool = true);

  // Conversão de binário para tipos inteiros
  template <class T>
    requires std::is_integral_v<T>
  std::array<std::byte, sizeof(T)> para_bin(const T&, std::endian = std::endian::native);

  template <class T>
    requires std::is_integral_v<T>
  T bin_para(const std::array<std::byte, sizeof(T)>&, std::endian = std::endian::native);

  // Transforma de byte para string
  std::string_view bin_to_strv(std::span<const std::byte>);
  std::string bin_to_str(std::span<const std::byte>);
  std::span<const std::byte> strv_to_bin(std::string_view);
}

#endif
// NES__BYTE_OP_H
