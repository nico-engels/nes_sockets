#include "unix_socket.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <netdb.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include "net_exc.h"
#include "socket_util.h"
#include "cfg.h"
using namespace std;
using namespace std::chrono_literals;
using namespace nes::net;
using namespace nes;

namespace nes::so {

  constexpr int SOCKET_INVALIDO = -1;
  constexpr int SOCKET_ERRO = -1;

  unix_socket::unix_socket()
    : m_unix_sd { SOCKET_INVALIDO }
  {

  }

  unix_socket::~unix_socket()
  {
    // Fecha o socket se existente
    if (m_unix_sd != SOCKET_INVALIDO)
    {
      // Termina as conexões
      shutdown(m_unix_sd, SHUT_RDWR);

      // Libera os recursos do Socket
      close(m_unix_sd);
    }
  }

  unix_socket::unix_socket(unix_socket&& outro) noexcept
    : m_unix_sd { outro.m_unix_sd }
    , m_end_ipv4 { move(outro.m_end_ipv4) }
    , m_porta_ipv4 { outro.m_porta_ipv4 }
  {
    outro.m_unix_sd = SOCKET_INVALIDO;
  }

  unix_socket& unix_socket::operator=(unix_socket&& outro) noexcept
  {
    m_unix_sd = outro.m_unix_sd;
    outro.m_unix_sd = SOCKET_INVALIDO;

    m_end_ipv4 = move(outro.m_end_ipv4);
    m_porta_ipv4 = outro.m_porta_ipv4;

    return *this;
  }

  const string& unix_socket::end_ipv4() const
  {
    return m_end_ipv4;
  }

  unsigned unix_socket::porta_ipv4() const
  {
    return m_porta_ipv4;
  }

  unix_socket::native_handle_type unix_socket::native_handle() const
  {
    return m_unix_sd;
  }

  void unix_socket::escutar(unsigned porta)
  {
    if (m_unix_sd != SOCKET_INVALIDO)
      throw runtime_error { "O socket já esta configurado." };

    // Inicializa o SOCKET pela API
    unique_ptr<int, function<void(int*)>> sock_serv = {
      new int { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) },
      [](int* p) { if (*p != SOCKET_INVALIDO) { close(*p); } delete p; }
    };

    // Checa se foi criado com sucesso
    if (*sock_serv == SOCKET_INVALIDO)
      throw runtime_error { "Não foi possível criar um socket pela API do Unix!" };

    // Configuração para resolução do endereço
    struct sockaddr_in end_res;

    end_res.sin_family = AF_INET;
    end_res.sin_addr.s_addr = INADDR_ANY;
    end_res.sin_port = htons(static_cast<short unsigned>(porta));

    // Liga o servidor ao endereço passando a estrutura
    if (bind(*sock_serv, reinterpret_cast<struct sockaddr*>(&end_res), sizeof(end_res)) < 0)
      throw runtime_error { "Escutar na porta " + to_string(porta) + ". Código do erro " + to_string(errno) +
        ": '" + strerror(errno) + "'." };

    // Define o socket servidor como assíncrono
    int marcadores = fcntl(*sock_serv, F_GETFL, 0);
    if (marcadores < 0 || fcntl(*sock_serv, F_SETFL, marcadores | O_NONBLOCK) != 0)
      throw runtime_error { "Não foi possível indicar o socket como assíncrono!" };

    // Chamada que configura o Socket para escutar pórem não o bloqueia
    // SOMAXCONN = Máx de conexões na fila
    if (listen(*sock_serv, SOMAXCONN) < 0)
      throw runtime_error { "Não iniciou a porta " + to_string(porta) + "!" };

    // Tudo em riba pode alterar a classe
    m_unix_sd = *sock_serv.release();
    m_end_ipv4 = "0.0.0.0";
    m_porta_ipv4 = porta;
  }

  bool unix_socket::conectado() const
  {
    return m_unix_sd != SOCKET_INVALIDO && m_end_ipv4 != "0.0.0.0" && m_porta_ipv4 != 0;
  }

  bool unix_socket::escutando() const
  {
    return m_unix_sd != SOCKET_INVALIDO && m_end_ipv4 == "0.0.0.0" && m_porta_ipv4 != 0;
  }

  bool unix_socket::ha_cliente()
  {
    // Validações
    if (!this->escutando())
      return false;

    // Checa se há algo para ler
    pollfd fd_sock;
    fd_sock.fd = m_unix_sd;
    fd_sock.events = POLLIN;

    if (poll(&fd_sock, 1, 0) <= 0)
     return false;

    // Verifica o campo de retorno
    if (fd_sock.revents & POLLIN)
      return true;

    return false;
  }

  optional<unix_socket> unix_socket::aceitar()
  {
    // Validações
    if (!this->escutando())
      throw runtime_error { "O socket não está inicializado para aceitar conexões!" };

    sockaddr_in cliente_info {};
    socklen_t size = sizeof(cliente_info);

    // Tenta aceitar alguma conexão na fila
    int socket_cli = accept(m_unix_sd, reinterpret_cast<sockaddr*>(&cliente_info), &size);
    if (socket_cli == SOCKET_INVALIDO)
    {
      // Como o socket não bloqueia verifica se tem não há conexão na fila
      if (errno == EWOULDBLOCK)
        return nullopt;
      else
        throw runtime_error { "Não foi possível aceitar a conexão!" };
    }
    else
    {
      // Cria o novo socket recebido
      unix_socket ret;

      // Dados do cliente
      ret.m_unix_sd = socket_cli;
      ret.m_end_ipv4 = inet_ntoa(cliente_info.sin_addr);
      ret.m_porta_ipv4 = ntohs(cliente_info.sin_port);

      // Define o socket servidor como assíncrono
      int marcadores = fcntl(socket_cli, F_GETFL, 0);
      if (marcadores < 0 || fcntl(socket_cli, F_SETFL, marcadores | O_NONBLOCK) != 0)
        throw runtime_error { "Não foi possível indicar o socket como assíncrono!" };

      return { move(ret) };
    }
  }

  void unix_socket::conectar(string endereco, unsigned porta)
  {
    if (m_unix_sd != SOCKET_INVALIDO)
      throw runtime_error { "O socket já esta configurado." };

    // Inicializa o SOCKET pela API
    unique_ptr<int, function<void(int*)>> socket_cli = {
      new int { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) },
      [](int* p) { if (*p != SOCKET_INVALIDO) { close(*p); } delete p; }
    };

    // Checa se foi criado com sucesso
    if (*socket_cli == SOCKET_INVALIDO)
      throw runtime_error { "Não foi possível criar um socket pela API do Unix!" };

    // Configuração para resolução do endereço
    struct addrinfo end_res_cfg;

    memset(&end_res_cfg, 0, sizeof(end_res_cfg));

    end_res_cfg.ai_family = AF_INET;
    end_res_cfg.ai_socktype = SOCK_STREAM;
    end_res_cfg.ai_protocol = IPPROTO_TCP;
    end_res_cfg.ai_flags = AI_PASSIVE;

    // Resolução
    struct addrinfo *end_res;
    if (getaddrinfo(endereco.data(), to_string(porta).c_str(), &end_res_cfg, &end_res))
      throw runtime_error { "Erro na resolução do getaddrinfo()!" };

    // RAAI da estrutura do endereço
    unique_ptr<struct addrinfo, function<void(struct addrinfo*)>> endResolvidoPtr {
      end_res,
      [](struct addrinfo* p) { freeaddrinfo(p); }
    };

    // Liga o servidor ao endereço passando a estrutura
    if (connect(*socket_cli, end_res->ai_addr, static_cast<socklen_t>(end_res->ai_addrlen)) != 0)
      throw runtime_error { "Não no endereço '" + endereco + ":" + to_string(porta) + "' !" };

    // Define o socket servidor como assíncrono
    int marcadores = fcntl(*socket_cli, F_GETFL, 0);
    if (marcadores < 0 || fcntl(*socket_cli, F_SETFL, marcadores | O_NONBLOCK) != 0)
      throw runtime_error { "Não foi possível indicar o socket como assíncrono!" };

    // Tudo em riba pode alterar a classe
    m_unix_sd = *socket_cli.release();
    m_end_ipv4 = move(endereco);
    m_porta_ipv4 = porta;
  }

  void unix_socket::desconectar()
  {
    if (this->conectado())
    {
      // Termina as conexões
      shutdown(m_unix_sd, SHUT_RDWR);

      // Libera os recursos do Socket
      close(m_unix_sd);

      m_unix_sd = SOCKET_INVALIDO;
      m_end_ipv4 = "0.0.0.0";
      m_porta_ipv4 = 0;
    }
  }

  void unix_socket::enviar(span<const byte> dados)
  {
    // Validação
    if (!this->conectado())
      throw runtime_error { "O socket precisa estar conectado para enviar dados!" };

    if (dados.size() == 0)
      return;

    size_t tentativas = 0;
    auto intervalo = cfg::net::intervalo_passo;
    for (ssize_t i = 0;
         i < static_cast<ssize_t>(dados.size());
         i += min(static_cast<ssize_t>(dados.size()) - i, static_cast<ssize_t>(cfg::net::bloco))) {
      ssize_t ret = send(m_unix_sd,
                         reinterpret_cast<const char*>(dados.data() + i),
                         min(dados.size() - static_cast<size_t>(i), cfg::net::bloco),
                         0);
      if (ret == -1)
      {
        // Falta de espaço no buffer da uma segunda chance
        if (tentativas < cfg::net::tentativas_max && errno == EWOULDBLOCK)
        {
          this_thread::sleep_for(intervalo);
          tentativas++;
          i -= cfg::net::bloco;

          // Baseado nas tentativas calcula o próximo intervalo
          intervalo = calc_intervalo_proporcional(tentativas);
        }
        else
          throw runtime_error { "Troca de dados no socket. Código erro: " + to_string(errno) + " - " + strerror(errno) };
      }
      else
      {
        if (ret < static_cast<ssize_t>(min(static_cast<ssize_t>(dados.size()) - i, static_cast<ssize_t>(cfg::net::bloco))))
          i -= min(static_cast<ssize_t>(dados.size()) - i, static_cast<ssize_t>(cfg::net::bloco)) - ret;
        tentativas = 0;
        intervalo = cfg::net::intervalo_passo;
      }
    }

  }

  vector<byte> unix_socket::receber()
  {
    // Validação
    if (!this->conectado())
      throw runtime_error { "Não conectado para receber dados!" };

    array<byte, cfg::net::bloco> dados;
    vector<byte> ret;

    // Enquanto houver dados a receber ou a conexão estiver ativa
    while (true)
    {
      auto qtde = recv(m_unix_sd, reinterpret_cast<void*>(dados.data()), dados.size(), 0);

      // Recebe os dados do socket, não bloqueia
      if (qtde == SOCKET_ERRO)
      {
        // Sem dados a receber, mas com conexão ativa
        if (errno == EWOULDBLOCK)
          break;
        else
          throw runtime_error { "Erro no recebimento! Código de erro: " + to_string(errno) + "!" };
      }
      else if (qtde == 0)
      {
        // Foi fechado o socket, se possui dados retorna
        if (ret.size() > 0)
          break;

        throw socket_desconectado { "Socket foi fechado normalmente!" };
      }

      // Vai guardando os dados recebidos na variável
      ret.insert(ret.end(), dados.begin(), dados.begin() + qtde);
    }

    return ret;
  }

}
