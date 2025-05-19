export module byte_op;

import std;
import nes_exc;

// Declaração
export namespace nes {

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

// Implementação
using namespace std;
namespace rng = std::ranges;

namespace nes {

  // Base64 Alphabet
  static const array<const char, 64> alf_base64 {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
  };

  string to_base64(string_view data, bool nl /*= true*/, bool pad /*= true*/)
  {
    return to_base64(as_bytes(span { data }), nl, pad);
  }

  string to_base64(span<const byte> data, bool nl /*= true*/, bool pad /*= true*/)
  {
    string ret;

    // Final string size calc
    // 1 - 3:4 Proportion
    auto final_size = (data.size() * 8) / 6 + 1 *
                      (((data.size() * 8) % 6) > 0);
    // 2 - Must multiple of 4
    final_size += ((final_size % 4) ? 4 - (final_size % 4) : 0);
    // 3 - New line
    if (nl)
      final_size += final_size/76;

    ret.reserve(final_size);

    // Auxiliar variables
    auto it = data.begin();
    byte curr_byte;
    byte next_byte;

    for (unsigned int i = 0; it != data.end(); ++i) {

      curr_byte = *it;
      next_byte = it + 1 != data.end() ? *(it + 1) : byte { 0 };

      // Extract the 6 bits to base64 lookup table
      switch (i % 4) {
        case 0:
          // curr_byte: xxxx xx00 next_byte: 0000 0000
          ret += alf_base64[to_integer<size_t>(curr_byte >> 2)];
          break;
        case 1:
          // curr_byte: 0000 00xx next_byte: xxxx 0000
          ret += alf_base64[to_integer<size_t>(((curr_byte & byte { 3 }) << 4) | (next_byte >> 4))];
          ++it;
          break;
        case 2:
          // curr_byte: 0000 xxxx next_byte: xx00 0000
          ret += alf_base64[to_integer<size_t>(((curr_byte & byte { 15 }) << 2) | (next_byte >> 6))];
          ++it;
          break;
        case 3:
          // curr_byte: 00xx xxxx next_byte: 0000 0000
          ret += alf_base64[to_integer<size_t>(curr_byte & byte { 63 })];
          ++it;
          break;
      }

      if (!((i + 1) % 76) && nl)
        ret += '\n';
    }

    // Padding if setted
    if (ret.size() < final_size && pad)
      ret += string(final_size - ret.size(), '=');

    return ret;
  }

  vector<byte> base64_to(string_view data, bool pad /*= true*/)
  {
    size_t data_size = static_cast<size_t>(rng::count_if(data, [](auto& b) { return b != '\r' && b != '\n'; }));
    if (pad && data_size % 4)
      throw nes_exc { "Base64 Converter Error, the data size must be multiple of 4!" };

    // Final vector size calc
    vector<byte> ret;
    ret.reserve(data_size * 6 / 8);

    // Auxiliar variables
    auto it = data.begin();
    byte curr_byte;
    byte prev_byte {};

    for (int i = 0; it != data.end() && *it != '='; ++i, ++it) {

      // Lookup
      auto it_base64 = rng::find(alf_base64, *it);
      if (it_base64 == alf_base64.end())
        throw nes_exc { "Base64 Converter Error, '{}' char out of Base64 Character set.", *it };

      curr_byte = static_cast<byte>(distance(alf_base64.begin(), it_base64));
      switch (i % 4)
      {
        case 0:
          prev_byte = curr_byte;
          break;
        case 1:
          // prev_byte xxxx xx00 curr_byte 0000 00xx (yyyy)
          ret.push_back(static_cast<byte>((prev_byte << 2) | (curr_byte >> 4)));
          prev_byte = curr_byte & byte { 15 };
          break;
        case 2:
          // prev_byte xxxx 0000 curr_byte 0000 xxxx (yy)
          ret.push_back(static_cast<byte>((prev_byte << 4) | (curr_byte >> 2)));
          prev_byte = curr_byte & byte { 3 };
          break;
        case 3:
          // prev_byte xx00 0000 curr_byte 00xx xxxx
          ret.push_back(static_cast<byte>((prev_byte << 6) | curr_byte));
          break;
      }
    }

    return ret;
  }

  template <>
  array<byte, 1> to_bin_arr(const int8_t& i, endian)
  {
    return { static_cast<byte>(i) };
  }

  template <>
  array<byte, 1> to_bin_arr(const uint8_t& i, endian)
  {
    return { static_cast<byte>(i) };
  }

  template <>
  array<byte, 2> to_bin_arr(const int16_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i), static_cast<byte>(i >> 8) };
    else
      return { static_cast<byte>(i >> 8), static_cast<byte>(i) };
  }

  template <>
  array<byte, 2> to_bin_arr(const uint16_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i), static_cast<byte>(i >> 8) };
    else
      return { static_cast<byte>(i >> 8), static_cast<byte>(i) };
  }

  template <>
  array<byte, 4> to_bin_arr(const int32_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i),       static_cast<byte>(i >> 8),
               static_cast<byte>(i >> 16), static_cast<byte>(i >> 24) };
    else
      return { static_cast<byte>(i >> 24), static_cast<byte>(i >> 16),
               static_cast<byte>(i >> 8),  static_cast<byte>(i) };
  }

  template <>
  array<byte, 4> to_bin_arr(const uint32_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i),       static_cast<byte>(i >> 8),
               static_cast<byte>(i >> 16), static_cast<byte>(i >> 24) };
    else
      return { static_cast<byte>(i >> 24), static_cast<byte>(i >> 16),
               static_cast<byte>(i >> 8),  static_cast<byte>(i) };
  }

  template <>
  array<byte, 8> to_bin_arr(const int64_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i),       static_cast<byte>(i >> 8),
               static_cast<byte>(i >> 16), static_cast<byte>(i >> 24),
               static_cast<byte>(i >> 32), static_cast<byte>(i >> 40),
               static_cast<byte>(i >> 48), static_cast<byte>(i >> 56) };
    else
      return { static_cast<byte>(i >> 56), static_cast<byte>(i >> 48),
               static_cast<byte>(i >> 40), static_cast<byte>(i >> 32),
               static_cast<byte>(i >> 24), static_cast<byte>(i >> 16),
               static_cast<byte>(i >> 8),  static_cast<byte>(i) };
  }

  template <>
  array<byte, 8> to_bin_arr(const uint64_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i),       static_cast<byte>(i >> 8),
               static_cast<byte>(i >> 16), static_cast<byte>(i >> 24),
               static_cast<byte>(i >> 32), static_cast<byte>(i >> 40),
               static_cast<byte>(i >> 48), static_cast<byte>(i >> 56) };
    else
      return { static_cast<byte>(i >> 56), static_cast<byte>(i >> 48),
               static_cast<byte>(i >> 40), static_cast<byte>(i >> 32),
               static_cast<byte>(i >> 24), static_cast<byte>(i >> 16),
               static_cast<byte>(i >> 8),  static_cast<byte>(i) };
  }

  template <>
  int8_t bin_arr_to(const array<byte, 1>& a, endian)
  {
    return to_integer<int8_t>(a[0]);
  }

  template <>
  uint8_t bin_arr_to(const array<byte, 1>& a, endian)
  {
    return to_integer<uint8_t>(a[0]);
  }

  template <>
  int16_t bin_arr_to(const array<byte, 2>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<int16_t>(to_integer<int16_t>(a[0]) + (to_integer<int16_t>(a[1]) << 8));
    else
      return static_cast<int16_t>(to_integer<int16_t>(a[1]) + (to_integer<int16_t>(a[0]) << 8));
  }

  template <>
  uint16_t bin_arr_to(const array<byte, 2>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<uint16_t>(to_integer<uint16_t>(a[0]) + (to_integer<uint16_t>(a[1]) << 8));
    else
      return static_cast<uint16_t>(to_integer<uint16_t>(a[1]) + (to_integer<uint16_t>(a[0]) << 8));
  }

  template <>
  int32_t bin_arr_to(const array<byte, 4>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<int32_t>( to_integer<int32_t>(a[0]       ) + (to_integer<int32_t>(a[1]) <<  8) +
                                  (to_integer<int32_t>(a[2]) << 16) + (to_integer<int32_t>(a[3]) << 24));
    else
      return static_cast<int32_t>( to_integer<int32_t>(a[3]       ) + (to_integer<int32_t>(a[2]) <<  8) +
                                  (to_integer<int32_t>(a[1]) << 16) + (to_integer<int32_t>(a[0]) << 24));
  }

  template <>
  uint32_t bin_arr_to(const array<byte, 4>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<uint32_t>(to_integer<uint32_t>(a[0]      ) + to_integer<uint32_t>(a[1] <<  8) +
                                   to_integer<uint32_t>(a[2] << 16) + to_integer<uint32_t>(a[3] << 24));
    else
      return static_cast<uint32_t>( to_integer<uint32_t>(a[3]       ) + (to_integer<uint32_t>(a[2]) <<  8) +
                                   (to_integer<uint32_t>(a[1]) << 16) + (to_integer<uint32_t>(a[0]) << 24));
  }

  template <>
  int64_t bin_arr_to(const array<byte, 8>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<int64_t>(
         to_integer<int64_t>(a[0]       ) + (to_integer<int64_t>(a[1]) <<  8) +
        (to_integer<int64_t>(a[2]) << 16) + (to_integer<int64_t>(a[3]) << 24) +
        (to_integer<int64_t>(a[4]) << 32) + (to_integer<int64_t>(a[5]) << 40) +
        (to_integer<int64_t>(a[6]) << 48) + (to_integer<int64_t>(a[7]) << 56)
      );
    else
      return static_cast<int64_t>(
         to_integer<int64_t>(a[7]       ) + (to_integer<int64_t>(a[6]) <<  8) +
        (to_integer<int64_t>(a[5]) << 16) + (to_integer<int64_t>(a[4]) << 24) +
        (to_integer<int64_t>(a[3]) << 32) + (to_integer<int64_t>(a[2]) << 40) +
        (to_integer<int64_t>(a[1]) << 48) + (to_integer<int64_t>(a[0]) << 56)
      );
  }

  template <>
  uint64_t bin_arr_to(const array<byte, 8>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<uint64_t>(
         to_integer<uint64_t>(a[0]       ) + (to_integer<uint64_t>(a[1]) <<  8) +
        (to_integer<uint64_t>(a[2]) << 16) + (to_integer<uint64_t>(a[3]) << 24) +
        (to_integer<uint64_t>(a[4]) << 32) + (to_integer<uint64_t>(a[5]) << 40) +
        (to_integer<uint64_t>(a[6]) << 48) + (to_integer<uint64_t>(a[7]) << 56)
      );
    else
      return static_cast<uint64_t>(
         to_integer<uint64_t>(a[7]       ) + (to_integer<uint64_t>(a[6]) <<  8) +
        (to_integer<uint64_t>(a[5]) << 16) + (to_integer<uint64_t>(a[4]) << 24) +
        (to_integer<uint64_t>(a[3]) << 32) + (to_integer<uint64_t>(a[2]) << 40) +
        (to_integer<uint64_t>(a[1]) << 48) + (to_integer<uint64_t>(a[0]) << 56)
      );
  }

  string_view bin_to_strv(span<const byte> bin)
  {
    // Convert from a byte view to a char view
    return { reinterpret_cast<const char*>(bin.data()), bin.size() };
  }

  string bin_to_str(span<const byte> bin)
  {
    // Convert from a byte view to a char view
    return { reinterpret_cast<const char*>(bin.data()), bin.size() };
  }

  span<const byte> strv_to_bin(string_view str)
  {
    // Convert from a char view to a byte view
    return { as_bytes(span { str })};
  }

}
