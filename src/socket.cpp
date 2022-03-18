#include "socket.h"

#include <algorithm>
#include <stdexcept>
#include <thread>
#include "net_exc.h"
#include "socket_util.h"
using namespace std;
using namespace std::chrono;
using namespace nes;
using namespace nes::so;

namespace nes::net {

  template <class S>
  socket_tmpl<S>::socket_tmpl(string endereco, unsigned porta)
  {
    this->conectar(move(endereco), porta);
  }

  template <class S>
  socket_tmpl<S>::socket_tmpl(S outro)
    : m_sock_so { move(outro) }
  {

  }

  template <class S>
  void socket_tmpl<S>::conectar(string endereco, unsigned porta)
  {
    m_sock_so.conectar(move(endereco), porta);
  }

  template <class S>
  void socket_tmpl<S>::desconectar()
  {
    m_sock_so.desconectar();
  }

  template <class S>
  const string& socket_tmpl<S>::end_ipv4() const
  {
    return m_sock_so.end_ipv4();
  }

  template <class S>
  unsigned socket_tmpl<S>::porta_ipv4() const
  {
    return m_sock_so.porta_ipv4();
  }

  template <class S>
  bool socket_tmpl<S>::conectado() const
  {
    return m_sock_so.conectado();
  }

  template <class S>
  socket_tmpl<S>::native_handle_type socket_tmpl<S>::native_handle() const
  {
    return m_sock_so.native_handle();
  }

  template <class S>
  void socket_tmpl<S>::enviar(span<const byte> dados)
  {
    m_sock_so.enviar(dados);
  }

  template <class S>
  void socket_tmpl<S>::enviar(string_view dados_str)
  {
    // Enquando não for adicionado std::span (C++20) faz assim
    const byte* b = reinterpret_cast<const byte*>(&*dados_str.begin());
    const byte* e = reinterpret_cast<const byte*>(&*dados_str.end());
    m_sock_so.enviar({ b, e });
  }

  template <class S>
  vector<byte> socket_tmpl<S>::receber()
  {
    return m_sock_so.receber();
  }

  template <class S>
  template <class R, class P>
  pair<vector<byte>, ptrdiff_t>
  socket_tmpl<S>::receber_ate_delim(span<const byte> delim, duration<R, P> tempo_exp, size_t tam_max)
  {
    using nes::net::receber_ate_delim;
    return receber_ate_delim(*this, delim, tempo_exp, tam_max);
  }

  template <class S>
  template <class R, class P>
  vector<byte> socket_tmpl<S>::receber_ate_tam(size_t tam, duration<R, P> tempo_exp)
  {
    using nes::net::receber_ate_tam;
    return receber_ate_tam(*this, tam, tempo_exp);
  }

  template <class S>
  template <class R, class P>
  vector<byte> socket_tmpl<S>::receber_ao_menos(size_t tam, duration<R, P> tempo_exp)
  {
    using nes::net::receber_ao_menos;
    return receber_ao_menos(*this, tam, tempo_exp);
  }

  template <class S>
  template <class R, class P>
  void socket_tmpl<S>::receber_resto(vector<byte>& dados, size_t tam, duration<R, P> tempo_exp)
  {
    using nes::net::receber_resto;
    receber_resto(*this, dados, tam, tempo_exp);
  }

  // Instanciações do template, devem vir ao final para funcionar no gcc e clang
  // Instanciar Template
  template class socket_tmpl<socket_so_impl>;

  // Instância as funções utilizadas
  template pair<vector<byte>, ptrdiff_t> socket_tmpl<socket_so_impl>::receber_ate_delim(span<const byte>, seconds, size_t);
  template pair<vector<byte>, ptrdiff_t> socket_tmpl<socket_so_impl>::receber_ate_delim(span<const byte>, milliseconds, size_t);
  template vector<byte> socket_tmpl<socket_so_impl>::receber_ate_tam(size_t, seconds);
  template vector<byte> socket_tmpl<socket_so_impl>::receber_ate_tam(size_t, milliseconds);
  template vector<byte> socket_tmpl<socket_so_impl>::receber_ao_menos(size_t, seconds);
  template void socket_tmpl<socket_so_impl>::receber_resto(vector<byte>&, size_t, seconds);
}
