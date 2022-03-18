#ifndef NES_NET__NET_EXC_H
#define NES_NET__NET_EXC_H

#include <stdexcept>

namespace nes::net {
  
  class net_exc : public std::runtime_error
    { using std::runtime_error::runtime_error; };
    
  // Exceções de HTTP
  class http_exc : public net_exc
    { using net_exc::net_exc; };

  // Domínio não encontrado
  class http_dominio_n_enc final : public http_exc
    { using http_exc::http_exc; };

  // Método HTTP
  // Inválido
  class http_metodo_invalido final : public http_exc
    { using http_exc::http_exc; };

  // Tamanho (Content-Length 411) requerido
  class http_n_tam_req final : public http_exc
    { using http_exc::http_exc; };

  // Tamanho da requisição muito bagual (413)
  class http_tam_grande final : public http_exc
    { using http_exc::http_exc; };

  // Acabou o tempo de espera para uma requisição
  class http_acabou_tempo final : public http_exc
    { using http_exc::http_exc; };
    
  // Exceções de socket
  class socket_exc : public net_exc
    { using net_exc::net_exc; };

  class socket_desconectado : public socket_exc
    { using socket_exc::socket_exc; };
    
   class socket_rec_expirado : public socket_exc
    { using socket_exc::socket_exc; };

   class socket_rec_excedente : public socket_exc
    { using socket_exc::socket_exc; };

}

#endif
// NES_NET__NET_EXC_H
