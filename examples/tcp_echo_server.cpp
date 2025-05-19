import std;

import socket;
import socket_serv;
using namespace std;
using namespace std::chrono_literals;
using namespace nes::net;
namespace vw = std::views;
namespace rng = std::ranges;

string_view application_name { "tcp_echo_server" };
const int default_port { 2700 };
const auto loop_sleep { 500ms };

void usage();
void serv_loop(stop_token, const int);
void print_hex_view(span<const byte>);

int main(const int argc, const char* argv[])
try {
    if (argc > 0)
      application_name = string_view { argv[0] };

    if (argc > 2)
      usage();

    int port_to_listen = default_port;
    if (argc == 2)
    {
      // If provided, parse the port
      const string_view port_argv(argv[1]);
      if (const auto rs = from_chars(port_argv.begin(), port_argv.end(), port_to_listen);
          rs.ec != std::errc{} || rs.ptr != port_argv.end())
      {
        usage();
        throw runtime_error { format("'{}' - Invalid port!", port_argv) };
      }
    }

    print("{} - echo tcp server using nes_sockets\n\n", application_name);
    {
      // Launch thread to handle the clients
      jthread serv_thr(serv_loop, port_to_listen);

      // Block until quit
      string option;
      while (cin >> option && option != "q");
    }
    println("* closing... bye");

} catch (const exception& e) {
    println("Exception thrown: {}", e.what());
} catch (...) {
    println("Exception thrown.");
}

void usage()
{
    print("{} [port]:\n"
          "  Send everything back to the calling socket and print the formatted hex data to the standart output.\n"
          "    [port]: Port listen to. Default to 2700.\n\n", application_name);
}

void serv_loop(stop_token st, const int port_to_listen)
try {
    socket_serv s(port_to_listen);
    println("* listening to the port {} (q to terminate)...", port_to_listen);

    vector<socket> clients;
    while (!st.stop_requested())
    {
      bool received_event = false;
      if (auto cli = s.accept())
      {
        println("* new client {}:{} connected!", cli->ipv4_address(), cli->ipv4_port());
        clients.push_back(move(*cli));
        received_event = true;
      }

      vector<const socket*> bad_clients;
      for (auto& cli : clients) try {
        const auto data = cli.receive();

        if (!data.empty())
        {
          println("* client {}:{} sends {} bytes of data:", cli.ipv4_address(), cli.ipv4_port(), data.size());
          print_hex_view(data);
          cli.send(data);
          received_event = true;
        }
      } catch (...) {
        println("* client {}:{} disconnected!", cli.ipv4_address(), cli.ipv4_port());
        bad_clients.push_back(&cli);
        received_event = true;
      }

      if (!bad_clients.empty())
        erase_if(clients, [&](const auto& cli) { return rng::contains(bad_clients, &cli); });

      if (!received_event)
        this_thread::sleep_for(loop_sleep);
    }
} catch (const exception& e) {
    println("Server died. Exception thrown: {}", e.what());
    quick_exit(1);
} catch (...) {
    println("Server died. Exception thrown.");
    quick_exit(1);
}

void print_hex_view(span<const byte> data)
{
    const int col_width = 24;
    for (const auto chk : data | vw::chunk(col_width)) {
      for (const byte& b : chk)
        print("{:02X} ", to_integer<int>(b));

      if (rng::size(chk) < col_width)
        print("{:{}}", ' ', (col_width - rng::size(chk)) * 3);

      print(" | ");
      for (const byte& b : chk)
        print("{}", isprint(to_integer<int>(b)) ? to_integer<char>(b) : '.');
      println("");
    }
}