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

  // Base64 Converter based on RFC 4648
  std::string to_base64(std::span<const std::byte>, bool = true, bool = true);
  std::string to_base64(std::string_view, bool = true, bool = true);

  std::vector<std::byte> base64_to(std::string_view, bool = true);

  // Binary Converter to integral types
  template <class T>
    requires std::is_integral_v<T>
  std::array<std::byte, sizeof(T)> to_bin_arr(const T&, std::endian = std::endian::native);

  template <class T>
    requires std::is_integral_v<T>
  T bin_arr_to(const std::array<std::byte, sizeof(T)>&, std::endian = std::endian::native);

  // Transform from byte to string
  std::string_view bin_to_strv(std::span<const std::byte>);
  std::string bin_to_str(std::span<const std::byte>);
  std::span<const std::byte> strv_to_bin(std::string_view);
}

#endif
// NES__BYTE_OP_H
