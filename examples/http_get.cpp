#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include "socket.h"
#include "socket_serv.h"
#include "tls_socket.h"
#include "tls_socket_serv.h"
using namespace std;
using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace nes::net;

template <class I>
void print_hex_char_view(I, I);

int main()
try {

    cout << "Example program using nes_sockets\n";

    cout << "1. Send a http request to https://www.google.com using a tls socket\n";
    {
      tls_socket c("google.com", 443);

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: www.google.com\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Send request (" << req.size() << " bytes):" << endl;
      print_hex_char_view(req.begin(), req.end());
      cout << "\n";
      c.send(req);

      sleep_for(1s);

      auto resp = c.receive();
      cout << "Protocol " << c.tls_protocol() << "\n"
              "Cipher " << c.cipher() << "\n\n"
              "< Received data (" << resp.size() << " bytes):\n";
      print_hex_char_view(resp.begin(), resp.end());
      cout << "\n";
    }

    cout << "2. Send a http request to http://google.com using a socket\n";
    {
      socket s("google.com", 80);

      string_view req("GET / HTTP/1.1\r\n"
                      "Host: www.google.com\r\n"
                      "Connection: close\r\n"
                      "\r\n");

      cout << "> Send request (" << req.size() << " bytes):" << endl;
      print_hex_char_view(req.begin(), req.end());
      cout << "\n";
      s.send(req);

      sleep_for(1s);

      auto resp = s.receive();
      cout << "< Received data (" << resp.size() << " bytes):\n";
      print_hex_char_view(resp.begin(), resp.end());
      cout << "\n";
    }

    cout << "3. Initiate a listen secure server socket on localhost port 55555\n";
    {
      tls_socket_serv server(55555, "../examples/expired-localhost-public.pem", "../examples/expired-localhost-private.pem");

      tls_socket client_a("localhost", 55555);

      auto client_b = server.accept().value();

      // If connecting the server client and client in same thread need make manual TLS handshake
      same_thread_handshake(client_a, client_b);

      // Generate a 1KB data filled most with 0xDE
      vector<byte> data(1'024, byte { 0xDE });
      for (int i = 0; i < 200; i++)
        data[i + 50] = static_cast<byte>(i);

      // Send the data
      client_a.send(data);

      // Receive the data until the exact expected size, throws if timeout (10 seconds)
      const auto data_recv = client_b.receive_until_size(data.size(), 10s);

      if (data == data_recv)
        cout << "Sent and received 1KB in localhost secure socket.\n"
                "Protocol " << client_a.tls_protocol() << "\n"
                "Cipher " << client_a.cipher() << "\n";
      else
        cout << "Error the data is corrupted.\n";
      cout << "\n";
    }

    cout << "4. Initiate a listen server socket on localhost port 55556\n";
    {
      socket_serv server(55556);

      socket client_a("localhost", 55556);

      auto client_b = server.accept().value();

      // Generate a 1KB data filled most with 0xDE
      vector<byte> data(1'024, byte { 0xDE });
      for (int i = 0; i < 200; i++)
        data[i + 50] = static_cast<byte>(i);

      // Send the data
      client_a.send(data);

      // Receive the data until the exact expected size, throws if timeout (10 seconds)
      const auto data_recv = client_b.receive_until_size(data.size(), 10s);

      if (data == data_recv)
        cout << "Sent and received 1KB in localhost socket.\n";
      else
        cout << "Error the data is corrupted.\n";
    }


} catch (const exception& e) {
    cout << "Exception thrown: " << e.what() << '\n';
} catch (...) {
    cout << "Exception thrown.\n";
}

template <class I>
void print_hex_char_view(I b, I e)
{
    constexpr int size_cols = 24;
    cout << hex;
    while (b != e)
    {
      auto i = b;
      for (size_t j = 0; j < size_cols; j++) {
        if (i != e)
          cout << setw(2) << +static_cast<unsigned char>(*i++) << ' ';
        else
          cout << "   ";
      }
      cout << "| ";

      for (size_t j = 0; b != e && j < size_cols; j++, ++b)
        cout << static_cast<char>(isprint(static_cast<int>(*b)) ? static_cast<unsigned char>(*b) : '.');
      cout << '\n';
    }
    cout << dec;
}
