#ifndef NES__CFG_H
#define NES__CFG_H

#include <chrono>

namespace nes {

  // Definições Gerais de Ambiente
  struct cfg {
    struct net {
       // Esperas
       static constexpr std::chrono::milliseconds intervalo_passo = std::chrono::milliseconds { 25 };
       static constexpr std::chrono::milliseconds intervalo_max   = std::chrono::seconds { 1 };

       static constexpr std::chrono::milliseconds timeout         = std::chrono::seconds { 5 };

       // Tamanhos
       static constexpr std::size_t bloco = 8'192;

       // Tentativas
       static constexpr std::size_t tentativas_max = 1'200;
    };
    struct so {
      // Indica qual OS será utilizado no sistema
      #ifdef _WIN32
      static constexpr auto is_windows = true;
      #else
      static constexpr auto is_windows = false;
      #endif
    };
  };

}

#endif
// NES__CFG_H
