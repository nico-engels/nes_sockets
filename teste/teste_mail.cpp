// C:/msys64/mingw64/bin/g++ -Wall -Werror -std=c++2a -static teste_mail.cpp -o ..\bin\win\teste_mail.exe -I..\include ..\bin\win\nes_sockets.a
#include <algorithm>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <locale>
#include <span>
#include <thread>
#include "socket.h"
using namespace std;
using namespace std::literals;
using namespace std::this_thread;
using namespace nes;
using namespace nes::net;

template <class I>
void printa_str_bin(I, I);

string_view bins_to_strv(span<const byte> bin)
{
  // Converte de uma visão binária para string
  auto b = reinterpret_cast<const char*>(&*bin.begin());
  auto e = reinterpret_cast<const char*>(&*bin.end());
  return { b, e };
}

span<const byte> strv_to_bins(string_view str)
{
  // Converte de uma string para visão binária
  auto b = reinterpret_cast<const byte*>(&*str.begin());
  auto e = reinterpret_cast<const byte*>(&*str.end());
  return { b, e };
}

int main()
try {
    setlocale(LC_ALL, "Portuguese");

    socket s("expressomx.pr.gov.br", 25);
    cout << "Conectar expressomx.pr.gov.br:25\n";

    sleep_for(500ms);

    auto resp = s.receber();
    printa_str_bin(resp.begin(), resp.end());

    if (bins_to_strv(resp) != "220 SMTP\r\n")
      throw runtime_error { "esperava hello serv!" };

    cout << "EHLO celepar.pr.gov.br\n";
    s.enviar("EHLO celepar.pr.gov.br\r\n");

    sleep_for(500ms);
    resp = s.receber();
    printa_str_bin(resp.begin(), resp.end());

    if (resp.size() == 0)
      throw runtime_error { "sem reposta de ehlo!" };

    cout << "MAIL FROM: \"Teste\"\n";
    s.enviar("MAIL FROM: \"Teste\"\r\n");

    sleep_for(500ms);
    resp = s.receber();
    printa_str_bin(resp.begin(), resp.end());

    if (bins_to_strv(resp) != "250 2.1.0 Ok\r\n")
      throw runtime_error { "esperava OK from!" };

    cout << "RCPT TO: <nengels@celepar.pr.gov.br>\n";
    s.enviar("RCPT TO: <nengels@celepar.pr.gov.br>\r\n");

    sleep_for(500ms);
    resp = s.receber();
    printa_str_bin(resp.begin(), resp.end());

    if (bins_to_strv(resp) != "250 2.1.5 Ok\r\n")
      throw runtime_error { "esperava OK to!" };

    cout << "DATA\n";
    s.enviar("DATA\r\n");

    sleep_for(500ms);
    resp = s.receber();
    printa_str_bin(resp.begin(), resp.end());

    if (bins_to_strv(resp) != "354 End data with <CR><LF>.<CR><LF>\r\n")
      throw runtime_error { "esperava OK send!" };

    cout << "Isso é um teste!\n";
    s.enviar("Isso é um teste!\n\r\n.\r\n");

    sleep_for(500ms);
    resp = s.receber();
    printa_str_bin(resp.begin(), resp.end());

    if (bins_to_strv(resp) != "354 send message\r\n")
      throw runtime_error { "esperava OK send!" };


} catch (const exception& e) {
    cout << "Exceção lançada: " << e.what() << '\n';
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