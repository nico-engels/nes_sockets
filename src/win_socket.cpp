#include "win_socket.h"

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <thread>
#include "cfg.h"
#include "net_exc.h"
#include "socket_util.h"

// WINSOCK2 API
#ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x501
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace nes;
using namespace nes::net;

namespace nes::so {

  // Contador de instâncias para inicialização/finalização da biblioteca
  atomic<unsigned> contador_inst { 0 };

  // Inicialização/Finalização da biblioteca
  void inicializa_WSA();
  void finaliza_WSA();

  // Mensagens de erro
  string msg_err_str(int);

  // Escopo para o SOCKET
  class socket_raii {
    SOCKET m_handle;
  public:
    socket_raii(SOCKET handle) : m_handle { handle } {};
    ~socket_raii() { if (m_handle != INVALID_SOCKET) closesocket(m_handle); };
    SOCKET handle() const { return m_handle; };
    SOCKET release() { SOCKET s = m_handle; m_handle = INVALID_SOCKET; return s; };
  };

  // Escopo para addrinfo
  class addrinfo_raii {
    addrinfo* m_handle;
   public:
    addrinfo_raii(addrinfo* handle) : m_handle { handle } {};
    ~addrinfo_raii() { if (m_handle) freeaddrinfo(m_handle); };
  };

  win_socket::win_socket()
    : m_winsocket { INVALID_SOCKET }
  {
    // Biblioteca dos sockets windows
    inicializa_WSA();
  }

  win_socket::win_socket(win_socket&& outro) noexcept
    : m_winsocket { outro.m_winsocket }
    , m_end_ipv4 { move(outro.m_end_ipv4) }
    , m_porta_ipv4 { outro.m_porta_ipv4 }
  {
    outro.m_winsocket = INVALID_SOCKET;

    // Biblioteca dos sockets windows
    inicializa_WSA();
  }

  win_socket& win_socket::operator=(win_socket&& outro) noexcept
  {
    m_winsocket = outro.m_winsocket;
    outro.m_winsocket = INVALID_SOCKET;

    m_end_ipv4 = move(outro.m_end_ipv4);
    m_porta_ipv4 = outro.m_porta_ipv4;

    return *this;
  }

  win_socket::~win_socket()
  {
    // Fecha o socket se existente
    if (m_winsocket != INVALID_SOCKET)
    {
      // Termina as conexões
      shutdown(m_winsocket, SD_BOTH);

      // Libera os recursos do Socket
      closesocket(m_winsocket);
    }

    // Verifica se deve finalizar o uso dos sockets
    finaliza_WSA();
  }

  const string& win_socket::end_ipv4() const
  {
    return m_end_ipv4;
  }

  unsigned win_socket::porta_ipv4() const
  {
    return m_porta_ipv4;
  }

  win_socket::native_handle_type win_socket::native_handle() const
  {
    return m_winsocket;
  }

  void win_socket::escutar(unsigned porta)
  {
    if (m_winsocket != INVALID_SOCKET)
      throw runtime_error { "win_socket::escutar() o socket já está configurado." };

    // Inicializa o SOCKET pela API
    socket_raii sock_serv { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };

    // Checa se foi criado com sucesso
    if (sock_serv.handle() == INVALID_SOCKET)
      throw runtime_error { "win_socket::escutar() erro ao criar uma socket pela API do Windows!" };

    // Configuração para resolução do endereço
    addrinfo end_res_cfg;

    memset(&end_res_cfg, 0, sizeof(end_res_cfg));

    end_res_cfg.ai_family = AF_INET;
    end_res_cfg.ai_socktype = SOCK_STREAM;
    end_res_cfg.ai_protocol = IPPROTO_TCP;
    end_res_cfg.ai_flags = AI_PASSIVE;

    // Resolução
    addrinfo *end_res;
    if (getaddrinfo(NULL, to_string(porta).c_str(), &end_res_cfg, &end_res))
      throw runtime_error { "win_socket::escutar() erro na resolução do WinSock2 getaddrinfo()!" };

    // RAAI da estrutura do endereço
    addrinfo_raii addr { end_res };

    // Liga o servidor ao endereço passando a estrutura
    if (::bind(sock_serv.handle(), end_res->ai_addr, static_cast<int>(end_res->ai_addrlen)))
      throw runtime_error { "win_socket::escutar() escutar na porta " + to_string(porta) + " !" };

    // Define o socket servidor como assíncrono
    u_long iModo = 1;
    if (ioctlsocket(sock_serv.handle(), FIONBIO, &iModo))
      throw runtime_error { "win_socket::escutar() não foi possível indicar o socket como assíncrono!" };

    // Chamada que configura o Socket para escutar pórem não o bloqueia
    // SOMAXCONN = Máx de conexões na fila
    if (listen(sock_serv.handle(), SOMAXCONN))
      throw runtime_error { "win_socket::escutar() iniciar espera conexões " + to_string(porta) + "!" };

    // Tudo em riba pode alterar a classe
    m_winsocket = sock_serv.release();
    m_end_ipv4 = "0.0.0.0";
    m_porta_ipv4 = porta;
  }

  bool win_socket::conectado() const
  {
    return m_winsocket != INVALID_SOCKET && m_end_ipv4 != "0.0.0.0" && m_porta_ipv4 != 0;
  }

  bool win_socket::escutando() const
  {
    return m_winsocket != INVALID_SOCKET && m_end_ipv4 == "0.0.0.0" && m_porta_ipv4 != 0;
  }

  bool win_socket::ha_cliente()
  {
    // Validações
    if (!this->escutando())
      return false;

    // Estrutura para informar o socket para a info e flag de inicialização
    FD_SET fdset_socket;

    // Timeout
    static timeval timeout { 0L, 0L };

    // Inicializa
    FD_ZERO(&fdset_socket);
    FD_SET(m_winsocket, &fdset_socket);

    // Faz a chamada para verificar se não há nenhuma conexão
    if (select(0, &fdset_socket, nullptr, nullptr, &timeout) == SOCKET_ERROR)
      return false;

    // Se o socket está ainda no vetor existe conexões senão não
    return FD_ISSET(m_winsocket, &fdset_socket) != 0;
  }

  optional<win_socket> win_socket::aceitar()
  {
    // Validações
    if (!this->escutando())
      throw runtime_error { "Não inicializado para aceitar conexões!" };

    struct sockaddr_in cliente_info = { 0, 0, 0, 0, 0, 0, 0 };
    int size = sizeof(cliente_info);

    // Tenta aceitar alguma conexão na fila
    SOCKET socket_cli = accept(m_winsocket, reinterpret_cast<sockaddr*>(&cliente_info), &size);
    if (socket_cli == INVALID_SOCKET)
    {
      // Como o socket não bloqueia verifica se tem não há conexão na fila
      if (WSAGetLastError() == WSAEWOULDBLOCK)
        return nullopt;
      else
        throw runtime_error { "Não foi possível aceitar a conexão!" };
    }
    else
    {
      // Cria o novo socket recebido
      win_socket ret;
      ret.m_winsocket = socket_cli;
      //ret.m_end_ipv4 = inet_ntoa(cliente_info.sin_addr);
      // xxx.xxx.xxx.xxx
      ret.m_end_ipv4.resize(15);
      if (!inet_ntop(AF_INET, reinterpret_cast<const void*>(&cliente_info.sin_addr),
                     ret.m_end_ipv4.data(), ret.m_end_ipv4.size()))
        throw runtime_error { "inet_ntop falhou" };;

      ret.m_porta_ipv4 = ntohs(cliente_info.sin_port);

      // Seta como não bloqueia
      u_long iModo = 1;
      if (ioctlsocket(ret.m_winsocket, FIONBIO, &iModo))
        throw runtime_error { "Ioctlsocket falhou" };

      return { move(ret) };
    }
  }

  void win_socket::conectar(string endereco, unsigned porta)
  {
    if (m_winsocket != INVALID_SOCKET)
      throw runtime_error { "O socket já esta configurado." };

    // Inicializa o SOCKET pela API
    socket_raii socket_cli { socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) };

    // Checa se foi criado com sucesso
    if (socket_cli.handle() == INVALID_SOCKET)
      throw runtime_error { "Não foi possível criar um socket!" };

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
      throw runtime_error { "Erro na resolução do WinSock2 getaddrinfo()!" };

    // RAAI da estrutura do endereço
    addrinfo_raii addr { end_res };

    // Liga o servidor ao endereço passando a estrutura
    if (connect(socket_cli.handle(), end_res->ai_addr, static_cast<int>(end_res->ai_addrlen)))
      throw runtime_error { "Não foi possível conectar ao endereço '" + endereco + ":" + to_string(porta) + "'!" };

    // Define o socket servidor como assíncrono
    u_long iModo = 1;
    if (ioctlsocket(socket_cli.handle(), FIONBIO, &iModo))
      throw runtime_error { "Não foi possível indicar o socket como assíncrono!" };

    // Tudo em riba pode alterar a classe
    m_winsocket = socket_cli.release();
    m_end_ipv4 = move(endereco);
    m_porta_ipv4 = porta;
  }

  void win_socket::desconectar()
  {
    if (this->conectado())
    {
      // Termina as conexões
      shutdown(m_winsocket, SD_BOTH);

      // Libera os recursos do Socket
      closesocket(m_winsocket);

      m_winsocket = INVALID_SOCKET;
      m_end_ipv4 = { "0.0.0.0" };
      m_porta_ipv4 = 0;
    }
  }

  void win_socket::enviar(span<const std::byte> dados)
  {
    // Validação
    if (!this->conectado())
      throw runtime_error { "O socket precisa estar conectado para enviar dados!" };

    if (dados.size() == 0)
      return;

    size_t tentativas = 0;
    milliseconds intervalo = cfg::net::intervalo_passo;
    for (int i = 0;
         i < static_cast<int>(dados.size());
         i += static_cast<int>(min(dados.size() - i, cfg::net::bloco))) {
      int ret = send(m_winsocket,
                     reinterpret_cast<const char*>(dados.data() + i),
                     static_cast<int>(min(dados.size() - i, cfg::net::bloco)),
                     0);
      if (ret == SOCKET_ERROR)
      {
        auto erro = WSAGetLastError();

        // Falta de espaço no buffer da uma segunda chance
        if (tentativas < cfg::net::tentativas_max && erro == WSAEWOULDBLOCK)
        {
          this_thread::sleep_for(intervalo);
          tentativas++;
          i -= static_cast<int>(cfg::net::bloco);

          // Baseado nas tentativas calcula o próximo intervalo
          intervalo = calc_intervalo_proporcional(tentativas);
        }
        else if (erro == WSAECONNABORTED)
          throw socket_desconectado { "Erro no envio de dados, socket foi fechado pelo destino!" };
        else
          throw runtime_error { "Troca de dados. Cód. Erro: " + msg_err_str(erro) };
      }
      else
      {
        if (ret < static_cast<int>(min(dados.size() - i, cfg::net::bloco)))
          i -= static_cast<int>(min(dados.size() - i, cfg::net::bloco) - ret);

        tentativas = 0;
        intervalo = cfg::net::intervalo_passo;
      }
    }
  }

  vector<std::byte> win_socket::receber()
  {
    // Validação
    if (!this->conectado())
      throw runtime_error { "Não conectado para receber dados!" };

    array<std::byte, cfg::net::bloco> dados;
    vector<std::byte> ret;

    // Enquanto houver dados a receber ou a conexão estiver ativa
    while (true)
    {
      auto qtde = recv(m_winsocket, reinterpret_cast<char*>(dados.data()), static_cast<int>(dados.size()), 0);

      // Recebe os dados do socket, não bloqueia
      if (qtde == SOCKET_ERROR)
      {
        auto erro = WSAGetLastError();

        // Sem dados a receber, mas com conexão ativa
        if (erro == WSAEWOULDBLOCK)
          break;
        else
          throw runtime_error { "Erro no recebimento. Código: " + msg_err_str(erro) + "!" };
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

  void inicializa_WSA()
  {
    // Se é a primeira instância da aplicação inicializa
    if (contador_inst++ == 0)
    {
      WSADATA wsaData;

      // Versão da WS2_32.Lib 2.2
      auto retorno = WSAStartup(MAKEWORD(2, 2), &wsaData);

      // Valida retorno
      if (retorno)
        throw runtime_error { "Erro inicialização WSA: " + msg_err_str(retorno) + "!" };

      // Valida versão
      if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
      {
        // Descarrega, pois é a versão errada
        WSACleanup();
        throw runtime_error { "WSA Versão exigida: 2.2. Versão Retornada: " + to_string(LOBYTE(wsaData.wVersion)) + "." +
          to_string(HIBYTE(wsaData.wVersion)) + "!" };
      }
    }
  }

  void finaliza_WSA()
  {
    if (--contador_inst == 0)
      WSACleanup();
  }

  string msg_err_str(int err_code)
  {
    array<char, 1024> buff {};
    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
      err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buff.data(), static_cast<DWORD>(buff.size()), NULL) == 0)
      return to_string(err_code) + " - msg_err_str! Erro retornado: " + to_string(WSAGetLastError());
    else
      return to_string(err_code) + " - " + buff.data();
  }

}