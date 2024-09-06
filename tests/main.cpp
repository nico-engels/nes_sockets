#include <filesystem>
#include <iostream>
#include "nes_exc.h"
#include "qtest.h"
using namespace std;
using namespace std::filesystem;
using namespace nes;
using namespace nes::test;

void set_default_directory();

// Tests
void test__socket();
void test__tls_socket();

int main()
try {
    // The test suite needs to be in the root folder nes_socket
    set_default_directory();

    cout << "Automated Test Suite\n"
         << "Default path dir: " << current_path() << endl;

    // Test entry points
    test__socket();
    test__tls_socket();

    //qtest::print_summary(print_options::only_errors);
    qtest::print_summary(print_options::all);

} catch (exception& e) {
    cout << "Error: " << e.what() << "\n\n";
    qtest::print_last();
    return 1;

} catch (...) {
    cout << "Unhandled exception!!!\n\n";
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

}

void test__tls_socket()
{

}