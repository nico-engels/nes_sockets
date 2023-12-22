#include "byte_op.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <stdexcept>
using namespace std;
namespace rng = std::ranges;

namespace nes {

  // Alfabeto base64
  static const array<const char, 64> alf_base64 {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
  };

  string para_base64(string_view dados, bool nl /*= true*/, bool pad /*= true*/)
  {
    return para_base64(as_bytes(span { dados }), nl, pad);
  }

  string para_base64(span<const byte> dados, bool nl /*= true*/, bool pad /*= true*/)
  {
    string ret;

    // Cálculo do tamanho da string final
    // 1 - Relação 3:4
    auto tam_final = (dados.size() * 8) / 6 + 1 *
                     (((dados.size() * 8) % 6) > 0);
    // 2 - Deve ser multiplo de 4
    tam_final += ((tam_final % 4) ? 4 - (tam_final % 4) : 0);
    // 3 - Retornos de Linhas
    if (nl)
      tam_final += tam_final/76;

    ret.reserve(tam_final);

    // Variáveis auxiliares
    auto it = dados.begin();
    byte bite, bite_prox;

    for (unsigned int i = 0; it != dados.end(); ++i) {

      bite = *it;
      bite_prox = it + 1 != dados.end() ? *(it + 1) : byte { 0 };

      switch (i % 4) {
        case 0:
          // bite: xxxx xx00 bite_prox: 0000 0000
          ret += alf_base64[to_integer<size_t>(bite >> 2)];
          break;
        case 1:
          // bite: 0000 00xx bite_prox: xxxx 0000
          ret += alf_base64[to_integer<size_t>(((bite & byte { 3 }) << 4) | (bite_prox >> 4))];
          ++it;
          break;
        case 2:
          // bite: 0000 xxxx bite_prox: xx00 0000
          ret += alf_base64[to_integer<size_t>(((bite & byte { 15 }) << 2) | (bite_prox >> 6))];
          ++it;
          break;
        case 3:
          // bite: 00xx xxxx bite_prox: 0000 0000
          ret += alf_base64[to_integer<size_t>(bite & byte { 63 })];
          ++it;
          break;
      }

      if (!((i + 1) % 76) && nl)
        ret += '\n';
    }

    // Se necessário insere os carac para que a sequência seja multipla de 4
    if (ret.size() < tam_final && pad)
      ret += string(tam_final - ret.size(), '=');

    return ret;
  }

  vector<byte> de_base64(string_view dados, bool pad /*= true*/)
  {
    // Valida entrada
    size_t tam_dados = static_cast<size_t>(rng::count_if(dados, [](auto& b) { return b != '\r' && b != '\n'; }));
    if (pad && tam_dados % 4)
      throw runtime_error { "Erro conversão base64 não possui o tamanho total multiplo de 4!" };

    // Cálculo do tamanho final
    vector<byte> ret;
    ret.reserve(tam_dados * 6 / 8);

    // Variáveis auxiliares
    auto it = dados.begin();
    byte bit6 { 0x00 }, bit6_ant { 0x00 };

    for (int i = 0; it != dados.end() && *it != '='; ++i, ++it) {

      // Busca a posição do caractere
      auto it_base64 = rng::find(alf_base64, *it);
      if (it_base64 == alf_base64.end())
        throw runtime_error { "Erro conversão base64 fora do intervalo do alfabeto base64." };

      bit6 = static_cast<byte>(distance(alf_base64.begin(), it_base64));
      switch (i % 4)
      {
        case 0:
          bit6_ant = bit6;
          break;
        case 1:
          ret.push_back(static_cast<byte>((bit6_ant << 2) | (bit6 >> 4)));
          bit6_ant = bit6 & byte { 15 };
          break;
        case 2:
          ret.push_back(static_cast<byte>((bit6_ant << 4) | (bit6 >> 2)));
          bit6_ant = bit6 & byte { 3 };
          break;
        case 3:
          ret.push_back(static_cast<byte>((bit6_ant << 6) | bit6));
          break;
      }
    }

    return ret;
  }

  template <>
  array<byte, 1> para_bin(const int8_t& i, endian)
  {
    return { static_cast<byte>(i) };
  }

  template <>
  array<byte, 1> para_bin(const uint8_t& i, endian)
  {
    return { static_cast<byte>(i) };
  }

  template <>
  array<byte, 2> para_bin(const int16_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i), static_cast<byte>(i >> 8) };
    else
      return { static_cast<byte>(i >> 8), static_cast<byte>(i) };
  }

  template <>
  array<byte, 2> para_bin(const uint16_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i), static_cast<byte>(i >> 8) };
    else
      return { static_cast<byte>(i >> 8), static_cast<byte>(i) };
  }

  template <>
  array<byte, 4> para_bin(const int32_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i),       static_cast<byte>(i >> 8),
               static_cast<byte>(i >> 16), static_cast<byte>(i >> 24) };
    else
      return { static_cast<byte>(i >> 24), static_cast<byte>(i >> 16),
               static_cast<byte>(i >> 8),  static_cast<byte>(i) };
  }

  template <>
  array<byte, 4> para_bin(const uint32_t& i, endian e)
  {
    if (e == endian::little)
      return { static_cast<byte>(i),       static_cast<byte>(i >> 8),
               static_cast<byte>(i >> 16), static_cast<byte>(i >> 24) };
    else
      return { static_cast<byte>(i >> 24), static_cast<byte>(i >> 16),
               static_cast<byte>(i >> 8),  static_cast<byte>(i) };
  }

  template <>
  array<byte, 8> para_bin(const int64_t& i, endian e)
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
  array<byte, 8> para_bin(const uint64_t& i, endian e)
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
  int8_t bin_para(const array<byte, 1>& a, endian)
  {
    return to_integer<int8_t>(a[0]);
  }

  template <>
  uint8_t bin_para(const array<byte, 1>& a, endian)
  {
    return to_integer<uint8_t>(a[0]);
  }

  template <>
  int16_t bin_para(const array<byte, 2>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<int16_t>(to_integer<int16_t>(a[0]) + (to_integer<int16_t>(a[1]) << 8));
    else
      return static_cast<int16_t>(to_integer<int16_t>(a[1]) + (to_integer<int16_t>(a[0]) << 8));
  }

  template <>
  uint16_t bin_para(const array<byte, 2>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<uint16_t>(to_integer<uint16_t>(a[0]) + (to_integer<uint16_t>(a[1]) << 8));
    else
      return static_cast<uint16_t>(to_integer<uint16_t>(a[1]) + (to_integer<uint16_t>(a[0]) << 8));
  }

  template <>
  int32_t bin_para(const array<byte, 4>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<int32_t>( to_integer<int32_t>(a[0]       ) + (to_integer<int32_t>(a[1]) <<  8) +
                                  (to_integer<int32_t>(a[2]) << 16) + (to_integer<int32_t>(a[3]) << 24));
    else
      return static_cast<int32_t>( to_integer<int32_t>(a[3]       ) + (to_integer<int32_t>(a[2]) <<  8) +
                                  (to_integer<int32_t>(a[1]) << 16) + (to_integer<int32_t>(a[0]) << 24));
  }

  template <>
  uint32_t bin_para(const array<byte, 4>& a, endian e)
  {
    if (e == endian::little)
      return static_cast<uint32_t>(to_integer<uint32_t>(a[0]      ) + to_integer<uint32_t>(a[1] <<  8) +
                                   to_integer<uint32_t>(a[2] << 16) + to_integer<uint32_t>(a[3] << 24));
    else
      return static_cast<uint32_t>( to_integer<uint32_t>(a[3]       ) + (to_integer<uint32_t>(a[2]) <<  8) +
                                   (to_integer<uint32_t>(a[1]) << 16) + (to_integer<uint32_t>(a[0]) << 24));
  }

  template <>
  int64_t bin_para(const array<byte, 8>& a, endian e)
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
  uint64_t bin_para(const array<byte, 8>& a, endian e)
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
    // Converte de uma visão binária para string
    return { reinterpret_cast<const char*>(bin.data()), bin.size() };
  }

  string bin_to_str(span<const byte> bin)
  {
    // Converte de uma visão binária para string
    return { reinterpret_cast<const char*>(bin.data()), bin.size() };
  }

  span<const byte> strv_to_bin(string_view str)
  {
    // Converte de uma string para visão binária
    return { as_bytes(span { str })};
  }

}
