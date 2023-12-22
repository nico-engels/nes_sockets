// C:/msys64/mingw64/bin/g++ -Wall -Werror -std=c++2a -static teste_http_get.cpp -o ..\bin\teste_http_get.exe -I..\include ..\bin\nes_sockets.a -lssl -lcrypto -lws2_32
#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string>
#include <string_view>
#include <thread>
#include "socket.h"
#include "tls_socket.h"
using namespace std;
using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace nes::net;

template <class I>
void printa_str_bin(I, I);

int main()
try {
    setlocale(LC_ALL, "Portuguese");

    /*
    {
      socket c("www.solicitacaodesistema.celepar.parana", 80);

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: www.solicitacaodesistema.celepar.parana\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Enviar req (" << req.size() << " bytes):" << endl;
      printa_str_bin(req.begin(), req.end());
      c.enviar(req);

      sleep_for(1s);

      auto resp = c.receber();
      cout << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());
    }
    */
    /*{
      socket c("rmip.celepar.parana", 80);

      string_view req("GET /dados/RMIP_UUEL_201908 HTTP/1.1\r\n"
                      "Host: rmip.celepar.parana\r\n"
                      "Connection: close\r\n"
                      "Authorization: Basic bmVuZ2VsczpFbmdlbHMwNDkx\r\n"
                      "\r\n");

      cout << "> Enviar req (" << req.size() << " bytes):" << endl;
      printa_str_bin(req.begin(), req.end());
      c.enviar(req);

      sleep_for(250ms);

      auto resp = c.receber();
      cout << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());
    } */
/*
    {
      tls_socket c("www.solicitacaodesistema.celepar.parana", 443);

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: www.solicitacaodesistema.celepar.parana\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Enviar req (" << req.size() << " bytes):" << endl;
      printa_str_bin(req.begin(), req.end());
      c.enviar(req);

      sleep_for(1s);

      auto resp = c.receber();
      cout << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());

      sleep_for(1s);

      resp = c.receber();
      cout << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());
    }
*/
    /*
    {
      socket c("api.sologenic.org", 80);

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: api.sologenic.org\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Enviar req (" << req.size() << " bytes):" << endl;
      printa_str_bin(req.begin(), req.end());
      c.enviar(req);

      sleep_for(1s);

      auto resp = c.receber();
      cout << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());
    }
    */

    {
      tls_socket c("google.com", 443);

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: google.com\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Enviar req (" << req.size() << " bytes):" << endl;
      printa_str_bin(req.begin(), req.end());
      c.enviar(req);

      sleep_for(1s);

      auto resp = c.receber();
      cout << "Protocolo " << c.protocolo_tls() << "\n"
           << "Cifra " << c.cifra() << "\n"
           << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());
    }

    {
      // curl --http1.1 -4 -k -v https://sologenic.org
      tls_socket c("sologenic.org", 443);

      // Virtual host
      c.tls_ext_host_name("sologenic.org");

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: sologenic.org\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Enviar req (" << req.size() << " bytes):" << endl;
      printa_str_bin(req.begin(), req.end());
      c.enviar(req);

      sleep_for(1s);

      auto resp = c.receber();
      cout << "Protocolo " << c.protocolo_tls() << "\n"
           << "Cifra " << c.cifra() << "\n"
           << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());
    }

    /*
    {
      tls_socket c("127.0.0.1", 4433);

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Enviar req (" << req.size() << " bytes):" << endl;
      printa_str_bin(req.begin(), req.end());
      c.enviar(req);

      sleep_for(1s);

      auto resp = c.receber();
      cout << "Protocolo " << c.protocolo_tls() << "\n"
           << "Cifra " << c.cifra() << "\n"
           << "< Dados recebidos (" << resp.size() << " bytes):\n";
      printa_str_bin(resp.begin(), resp.end());
    }
    */

} catch (const exception& e) {
    cout << "Exceção lançada: " << e.what() << '\n';
} catch (...) {
    cout << "Exceção lançada\n";
}

template <class I>
void printa_str_bin(I b, I e)
{
    constexpr int tam = 24;
    cout << hex;
    while (b != e)
    {
      auto i = b;
      for (size_t j = 0; j < tam; j++) {
        if (i != e)
          cout << setw(2) << +static_cast<unsigned char>(*i++) << ' ';
        else
          cout << "   ";
      }
      cout << "| ";

      for (size_t j = 0; b != e && j < tam; j++, ++b)
        cout << static_cast<char>(isprint(static_cast<int>(*b)) ? static_cast<unsigned char>(*b) : '.');
      cout << '\n';
    }
    cout << dec;
}
