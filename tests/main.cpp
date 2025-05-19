import std;

import byte_op;
import nes_exc;
import net_exc;
import qtest;
import socket;
import socket_serv;
import tls_socket;
import tls_socket_serv;

using namespace std;
using namespace std::chrono_literals;
using namespace std::filesystem;
using namespace nes;
using namespace nes::net;
using namespace nes::test;

void set_default_directory();

// Tests
void test__socket();
void test__tls_socket();

int main()
try {
    // The test suite needs to be in the root folder nes_socket
    set_default_directory();

    println("Automated Test Suite\n"
            "Default path dir: '{}'", current_path().native());

    // Test entry points
    qtest::package("nes_sockets");
    test__socket();
    test__tls_socket();

    qtest::print_summary(print_options::only_errors);
    //qtest::print_summary(print_options::all);

} catch (exception& e) {
    println("Error: {}\n", e.what());
    qtest::print_last();
    return 1;

} catch (...) {
    println("Unhandled exception!!!\n");
    qtest::print_last();
    return 1;

}

void set_default_directory()
{
    const path padrao("nes_sockets/bin");

    auto cp = current_path();
    if (distance(cp.begin(), cp.end()) < 2)
      throw nes_exc { "The test suite needs to be in the root folder nes_sockets (current {})!",
        current_path().string() };

    // Check if is in the correct diretory
    if (equal(padrao.begin(), padrao.end(), prev(cp.end(), 2), cp.end()))
      return;

    // Try to find the folder in parent directory
    while (cp.stem().string() != "nes_sockets" && cp != cp.parent_path())
      cp = cp.parent_path();

    // Se é o diretório correto
    cp = cp / "bin";
    if (exists(cp))
      current_path(cp);
    else
      throw nes_exc { "The test suite needs to be in the root folder nes_sockets (current {})!",
        current_path().string() };
}

void test__socket()
{
    qtest::sub_package("nes::net::socket[_serv]");
    qtest::sub_package_title("constructor's socket[serv]");

    random_device rd;
    mt19937 gen(rd());

    {
      socket_serv a;

      qtest::is_false(a.is_listening());
      qtest::eq(a.ipv4_port(), size_t { 0 });
      qtest::is_false(a.has_client());

      try {
        a.accept();
        qtest::unreachable();
      } catch (const runtime_error&) {
        qtest::ok("a.accept(); runtime_error ok");
      } catch (...) {
        qtest::unreachable();
      }

      uniform_int_distribution<unsigned> port_distrib(50136, 50737);
      unsigned port_ran { port_distrib(gen) };

      a = socket_serv { port_ran };

      qtest::is_true(a.is_listening());
      qtest::eq(a.ipv4_port(), size_t { port_ran });
      qtest::is_false(a.has_client());
      qtest::eq(a.accept(), nullopt);

      socket b;

      qtest::eq(b.ipv4_address(), "0.0.0.0");
      qtest::eq(b.ipv4_port(), size_t { 0 });

      b = socket { "127.0.0.1", port_ran };

      qtest::eq(b.ipv4_address(), "127.0.0.1");
      qtest::eq(b.ipv4_port(), size_t { port_ran });

      this_thread::sleep_for(50ms);

      qtest::is_true(a.is_listening());
      qtest::is_true(a.has_client());
      qtest::is_true(a.accept().has_value());
      qtest::is_false(a.accept().has_value());
    }

    qtest::sub_package_title("localhost communication");

    {
      uniform_int_distribution<unsigned> port_distrib(50738, 51232);
      unsigned port_ran { port_distrib(gen) };

      socket_serv a(port_ran);
      qtest::is_true(a.is_listening());

      socket b("localhost", port_ran);

      this_thread::sleep_for(50ms);

      qtest::is_true(b.is_connected());
      qtest::eq(b.ipv4_address(), "127.0.0.1");
      qtest::is_true(a.has_client());

      auto oc = a.accept();
      qtest::is_true(oc);
      if (oc)
      {
        auto c = move(*oc);
        qtest::is_true(c.is_connected());
        qtest::eq(b.ipv4_address(), c.ipv4_address());

        c.send("abcd");
        auto data_recv = b.receive();
        qtest::eq(bin_to_strv(data_recv), "abcd");

        b.send("1234");
        b.send("5678");
        data_recv = c.receive();

        qtest::eq(bin_to_strv(data_recv), "12345678");

        vector<byte> data_send;
        for (char i = 0; i < 40; i++)
          data_send.push_back(static_cast<byte>(i));

        c.send(data_send);
        data_recv = b.receive();

        qtest::eq(data_recv, data_send);

        for (int i = 0; i < 500; i++)
          data_send.push_back(static_cast<byte>(i));

        b.send(data_send);
        data_recv = c.receive();

        qtest::eq(data_recv, data_send);

        for (int i = 0; i < 2000; i++)
          data_send.push_back(static_cast<byte>(i));
        b.send(data_send);

        // A little sleep until all SO round trip
        this_thread::sleep_for(50ms);
        data_recv = c.receive();

        qtest::eq(data_recv, data_send);

        c.send(data_send);
        c.send(data_send);
        c.send(data_send);
        c.send(data_send);
        data_send.insert(data_send.end(), data_send.begin(), data_send.end());
        data_send.insert(data_send.end(), data_send.begin(), data_send.end());
        this_thread::sleep_for(50ms);
        data_recv = b.receive();

        qtest::eq(data_recv, data_send);

        // I/O Socket utilities
        for (const auto i : { byte { 0x00 }, byte { 0x01 }, byte { 0xC0 },
                              byte { 0xC0 }, byte { 0x02 }, byte { 0x03 } })
          data_send.push_back(i);
        c.send(data_send);

        data_recv = b.receive_until_delimiter(vector { byte { 0xC0 }, byte { 0xC0 }}, 1s, 10'166).first;

        // If the two bytes after delimiter is missing
        size_t tam = 0;
        if (data_recv.size() != 10'166)
          tam = 10'166 - data_recv.size();

        c.send(data_send);
        data_recv = b.receive_until_size(10'166 + tam, 1s);

        qtest::eq(data_recv.size(), 10'166 + tam);

        // Test fail receive
        try {
          data_recv = c.receive_until_size(1, 50ms);
          qtest::unreachable();
        } catch (const socket_timeout&) {
          qtest::ok("c.receive_until_size(); socket_timeout ok");
        } catch (...) {
          qtest::unreachable();
        }

        // Test desconection
        { auto d = move(c); (void)d; }
        try {
          data_recv = b.receive();
          qtest::unreachable();
        } catch (const socket_disconnected&) {
          qtest::ok("b.receive(); socket_disconnected ok");
        } catch (...) {
          qtest::unreachable();
        }
      }
    }
}

void test__tls_socket()
{
    qtest::sub_package("nes::net::tls_socket[_serv]");
    qtest::sub_package_title("construtor's tls_socket[serv]");

    random_device rd;
    mt19937 gen(rd());

    {
      uniform_int_distribution<unsigned> port_distrib(5100, 5500);
      unsigned port_ran { port_distrib(gen) };

      try {
        tls_socket_serv a(port_ran, "../examples/expired-localhost-public.pem",
                                    "../examples/expired-localhost-private.pem");
      } catch (...) {
        qtest::unreachable();
      }

      tls_socket_serv a;

      qtest::is_false(a.is_listening());
      qtest::eq(a.ipv4_port(), size_t { 0 });
      qtest::is_false(a.has_client());

      try {
        a.accept();
        qtest::unreachable();
      } catch (const runtime_error&) {
        qtest::ok("a.accept(); runtime_error ok");
      } catch (...) {
        qtest::unreachable();
      }

      try {
        a.listen(port_ran, "../examples/expired-localhost-public.pem",
                           "../examples/expired-localhost-private.pem");
      } catch (...) {
        qtest::unreachable();
      }

      qtest::is_true(a.is_listening());
      qtest::eq(a.ipv4_port(), size_t { port_ran });
      qtest::is_false(a.has_client());

      tls_socket b;

      qtest::eq(b.ipv4_address(), "0.0.0.0");
      qtest::eq(b.ipv4_port(), size_t { 0 });
      qtest::eq(b.cipher(), "");

      try { b = tls_socket { "127.0.0.1", port_ran }; } catch (...) { qtest::unreachable(); }

      qtest::eq(b.ipv4_address(), "127.0.0.1");
      qtest::eq(b.ipv4_port(), size_t { port_ran });
      qtest::is_false(b.cipher().empty());

      qtest::is_true(a.is_listening());
      qtest::is_true(a.has_client());
      qtest::is_true(a.is_listening() && a.accept().has_value());
      qtest::is_false(a.is_listening() && a.accept().has_value());
    }

    qtest::sub_package_title("localhost communication");

    {
      uniform_int_distribution<unsigned> port_distrib(5501, 5599);
      unsigned port_ran { port_distrib(gen) };

      tls_socket_serv a;
      try {
        a = tls_socket_serv { port_ran, "../examples/expired-localhost-public.pem",
          "../examples/expired-localhost-private.pem" };
      } catch (...) { qtest::unreachable(); }
      qtest::is_true(a.is_listening());

      tls_socket b;
      try {
        b = tls_socket { "127.0.0.1", port_ran };
      } catch (...) { qtest::unreachable(); }

      this_thread::sleep_for(50ms);

      qtest::is_true(b.is_connected());
      qtest::is_true(a.has_client());

      optional<tls_socket> oc;
      try { oc = a.accept(); } catch (...) { qtest::unreachable(); }
      qtest::is_true(oc);
      if (oc)
      {
        auto c = move(*oc);
        qtest::is_true(c.is_connected());
        qtest::eq(b.ipv4_address(), c.ipv4_address());

        // In same thread need make manual TLS handshake
        same_thread_handshake(c, b);
        c.send("abcd");
        this_thread::sleep_for(50ms);
        auto dados_rec = b.receive();

        qtest::eq(bin_to_strv(dados_rec), "abcd");

        b.send("1234");
        b.send("5678");
        this_thread::sleep_for(50ms);
        dados_rec = c.receive();
        qtest::eq(bin_to_strv(dados_rec), "12345678");

        vector<byte> dados_env;
        for (char i = 0; i < 40; i++)
          dados_env.push_back(static_cast<byte>(i));
        c.send(dados_env);
        dados_rec = b.receive();

        qtest::eq(dados_rec, dados_env);

        for (int i = 0; i < 500; i++)
          dados_env.push_back(static_cast<byte>(i));
        b.send(dados_env);
        dados_rec = c.receive();

        qtest::eq(dados_rec, dados_env);

        for (int i = 0; i < 2000; i++)
          dados_env.push_back(static_cast<byte>(i));
        b.send(dados_env);

        // A little sleep until all SO round trip
        this_thread::sleep_for(50ms);
        dados_rec = c.receive();

        qtest::eq(dados_rec, dados_env);

        c.send(dados_env);
        c.send(dados_env);
        c.send(dados_env);
        c.send(dados_env);
        dados_env.insert(dados_env.end(), dados_env.begin(), dados_env.end());
        dados_env.insert(dados_env.end(), dados_env.begin(), dados_env.end());
        this_thread::sleep_for(50ms);

        dados_rec = b.receive();
        while (dados_rec.size() != 10'160)
        {
          auto dados = b.receive();

          if (dados.size() == 0)
            break;

          dados_rec.insert(dados_rec.end(), dados.begin(), dados.end());
        }
        qtest::eq(dados_rec, dados_env);

        // I/O Socket utilities
        for (auto i : { byte { 0x00 }, byte { 0x01 }, byte { 0xC0 }, byte { 0xC0 }, byte { 0x02 }, byte { 0x03 } })
          dados_rec.push_back(i);
        c.send(dados_rec);
        dados_rec = b.receive_until_delimiter(vector { byte { 0xC0 }, byte { 0xC0 }}, 1s, 10'166).first;

        // If the two bytes after delimiter is missing
        size_t tam = 0;
        if (dados_rec.size() != 10'166)
          tam = 10'166 - dados_rec.size();

        c.send(dados_rec);
        dados_rec = b.receive_until_size(10'166 + tam, 1s);

        qtest::eq(dados_rec.size(), 10'166 + tam);

        // Test fail receive
        try {
          dados_rec = c.receive_until_size(1, 50ms);
          qtest::unreachable();
        } catch (const socket_timeout&) {
          qtest::ok("c.receive_until_size(); socket_timeout ok");
        } catch (...) {
          qtest::unreachable();
        }

        // Test desconection
        { auto d = move(c); (void)d; }
        try {
          dados_rec = b.receive();
          qtest::eq(dados_rec.size(), size_t { 0 });

          dados_rec = b.receive();
          qtest::unreachable();
        } catch (const runtime_error&) {
          qtest::ok("b.receive(); runtime_error ok");
        } catch (...) {
          qtest::unreachable();
        }
      }
    }
}
