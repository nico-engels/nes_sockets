#include "socket_util.h"

#include <thread>
#include "cfg.h"
#include "net_exc.h"
#include "socket.h"
#include "tls_socket.h"
using namespace std;
using namespace std::chrono;

namespace nes::net {

  milliseconds calc_intervalo_proporcional(size_t tentativas)
  {
    // O intervalo cresce proporcionalmente sendo 0 == intervalo_passo e tentativas_max == intervalo_max
    double intervalo_dif = static_cast<double>((cfg::net::intervalo_max - cfg::net::intervalo_passo).count());
    double tentativas_prop = static_cast<double>(tentativas) / cfg::net::tentativas_max;
    return cfg::net::intervalo_passo + milliseconds { static_cast<milliseconds::rep>(intervalo_dif * tentativas_prop) };
  }

  // Funções de entrada io
  template <class S, class R, class P>
  pair<vector<byte>, ptrdiff_t> receber_ate_delim(S& sock, span<const byte> delim, duration<R, P> tempo_exp, size_t tam_max)
  {
    vector<byte> ret;

    // Fica continuamente tentando receber os dados
    auto inicio = steady_clock::now();
    while (steady_clock::now() - inicio < tempo_exp)
    {
      auto dados = sock.receber();

      // Se recebeu dados
      if (dados.size() > 0)
      {
        // Verificação para não receber muitooos dados
        if (ret.size() + dados.size() > tam_max)
          throw socket_rec_excedente { "Recebeu mais dados (" + to_string(ret.size() + dados.size()) +
            "B) que o máximo (" + to_string(tam_max) + "B)!" };

        // Adiciona ao retorno e procura o delimitador
        ret.insert(ret.end(), dados.begin(), dados.end());
        if (auto it = search(ret.begin(), ret.end(), delim.begin(), delim.end()); it != ret.end())
          return { ret, it - ret.begin() };

      }

      // Aguarda e incrementa a espera
      this_thread::sleep_for(cfg::net::intervalo_passo);
    }

    // Chegou aqui estourou o tempo
    throw socket_rec_expirado { "Tempo de espera " + to_string(tempo_exp.count()) + " expirado! "
      "Sem delimitador! Recebeu " + to_string(ret.size()) + " Bytes!" };
  }

  template <class S, class R, class P>
  vector<byte> receber_ate_tam(S& sock, size_t tam, duration<R, P> tempo_exp)
  {
    vector<byte> ret;

    // Fica continuamente tentando receber os dados
    auto inicio = steady_clock::now();
    while (steady_clock::now() - inicio < tempo_exp)
    {
      auto dados = sock.receber();

      // Se recebeu dados
      if (dados.size() > 0)
      {
        // Verificação para não receber muitooos dados
        if (ret.size() + dados.size() > tam)
          throw socket_rec_excedente { "Recebeu mais dados (" + to_string(ret.size() + dados.size()) +
            "B) que o tamanho especificado (" + to_string(tam) + "B)!" };

        // Adiciona ao retorno e verifica se atingiu o tamanho
        ret.insert(ret.end(), dados.begin(), dados.end());
        if (ret.size() == tam)
          return ret;
      }

      // Aguarda e incrementa a espera
      this_thread::sleep_for(cfg::net::intervalo_passo);
    }

    // Chegou aqui estourou o tempo
    throw socket_rec_expirado { "Tempo de espera " + to_string(tempo_exp.count()) + " expirado! "
      "Não chegou ao tamanho " + to_string(tam) + " Bytes! Recebeu " + to_string(ret.size()) + " Bytes!" };
  }

  template <class S, class R, class P>
  vector<byte> receber_ao_menos(S& sock, size_t tam, duration<R, P> tempo_exp)
  {
    vector<byte> ret;

    // Fica continuamente tentando receber os dados
    auto inicio = steady_clock::now();
    while (steady_clock::now() - inicio < tempo_exp)
    {
      auto dados = sock.receber();

      // Se recebeu dados
      if (dados.size() > 0)
      {
        // Adiciona ao retorno e verifica se atingiu o tamanho
        ret.insert(ret.end(), dados.begin(), dados.end());
        if (ret.size() >= tam)
          return ret;
      }

      // Aguarda e incrementa a espera
      this_thread::sleep_for(cfg::net::intervalo_passo);
    }

    // Chegou aqui estourou o tempo
    throw socket_rec_expirado { "Tempo de espera " + to_string(tempo_exp.count()) + " expirado! "
      "Não recebeu ao menos " + to_string(tam) + " Bytes ! Recebeu " + to_string(ret.size()) + " Bytes!" };
  }

  template <class S, class R, class P>
  void receber_resto(S& sock, vector<byte>& dados, size_t tam, duration<R, P> tempo_exp)
  {
    if (dados.size() < tam)
    {
      auto resto = receber_ate_tam(sock, tam - dados.size(), tempo_exp);
      dados.insert(dados.end(), resto.begin(), resto.end());
    }
  }

  // Instanciações do template, devem vir ao final para funcionar no gcc e clang
  // Instância as utilizações
  template pair<vector<byte>, ptrdiff_t> receber_ate_delim(socket&, span<const byte>, seconds, size_t);
  template pair<vector<byte>, ptrdiff_t> receber_ate_delim(socket&, span<const byte>, milliseconds, size_t);
  template vector<byte> receber_ate_tam(socket&, size_t, seconds);
  template vector<byte> receber_ate_tam(socket&, size_t, milliseconds);
  template vector<byte> receber_ao_menos(socket&, size_t, seconds);
  template void receber_resto(socket&, vector<byte>&, size_t, seconds);

  template pair<vector<byte>, ptrdiff_t> receber_ate_delim(tls_socket&, span<const byte>, seconds, size_t);
  template pair<vector<byte>, ptrdiff_t> receber_ate_delim(tls_socket&, span<const byte>, milliseconds, size_t);
  template vector<byte> receber_ate_tam(tls_socket&, size_t, seconds);
  template vector<byte> receber_ate_tam(tls_socket&, size_t, milliseconds);
  template vector<byte> receber_ao_menos(tls_socket&, size_t, seconds);
  template void receber_resto(tls_socket&, vector<byte>&, size_t, seconds);

}
