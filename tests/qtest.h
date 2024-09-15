#ifndef NES_TEST__QTEST_H
#define NES_TEST__QTEST_H

#include <iostream>
#include <ranges>
#include <source_location>
#include <sstream>
#include <string>

namespace nes::test {

  namespace detail {

    template <class T, class U>
    std::string print_values_test(T&& lhs, U&& rhs)
    {
      std::ostringstream oss;
      oss << "lhs: " << std::forward<T>(lhs) << '\n'
          << "rhs: " << std::forward<U>(rhs) << '\n';

      return oss.str();
    }

    template <class T>
    concept Streamable = requires(std::ostream& out, T a) {
      { out << a } -> std::same_as<std::ostream&>;
    };

    template <class T, class U>
      requires (!Streamable<T> || !Streamable<U>)
    std::string print_values_test(T&&, U&&)
    {
      std::ostringstream oss;
      oss << "lhs: [no representation]\n"
          << "rhs: [no representation]\n";

      return oss.str();
    }

  }

  enum class print_options { only_errors, all };
  enum class test_exec_status { ok, err, exc };

  class qtest
  {
    static void exec_add(test_exec_status, std::string, std::source_location);

  public:

    // Test organization
    static void package(std::string);
    static void sub_package(std::string);
    static void sub_package_title(std::string);

    // Test operations
    // Logical operations (==, !=, <, <=, >, >=)
    template <class T, class U>
    static void eq(T&& lhs, U&& rhs,
                   std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) == std::forward<U>(rhs) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void neq(T&& lhs, U&& rhs,
                    std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) != std::forward<U>(rhs) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void lt(T&& lhs, U&& rhs,
                   std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) < std::forward<U>(rhs) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void lteq(T&& lhs, U&& rhs,
                     std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) <= std::forward<U>(rhs) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void gt(T&& lhs, U&& rhs,
                   std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) > std::forward<U>(rhs) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void gteq(T&& lhs, U&& rhs,
                     std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) >= std::forward<U>(rhs) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    // Boolean operations
    template <class T>
    static void is_true(T&& op,
                        std::source_location l = std::source_location::current())
    {
      qtest::exec_add(static_cast<bool>(std::forward<T>(op)) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(static_cast<bool>(std::forward<T>(op)), true), std::move(l));
    }

    template <class T>
    static void is_false(T&& op,
                         std::source_location l = std::source_location::current())
    {
      qtest::exec_add(!static_cast<bool>(std::forward<T>(op)) ? test_exec_status::ok : test_exec_status::err,
                      detail::print_values_test(static_cast<bool>(std::forward<T>(op)), false), std::move(l));
    }

    // Flow operations
    static void unreachable(std::string msg,
                            std::source_location l = std::source_location::current())
    {
      qtest::exec_add(test_exec_status::err, "unreachable" + msg + "\n", std::move(l));
    }

    static void unreachable(std::exception_ptr ep = std::current_exception(),
                            std::source_location l = std::source_location::current())
    {
      try {
        if (ep)
          std::rethrow_exception(ep);
        qtest::exec_add(test_exec_status::err, "unreachable\n", std::move(l));
      } catch (const std::exception& e) {
        qtest::exec_add(test_exec_status::exc, std::string { "unreachable - exception: " } +
          e.what() + "\n", std::move(l));
      } catch (...) {
        qtest::exec_add(test_exec_status::exc, "unreachable - exception: other\n", std::move(l));
      }
    }

    static void ok(std::string msg,
                   std::source_location l = std::source_location::current())
    {
      qtest::exec_add(test_exec_status::ok, std::move(msg), std::move(l));
    }

    // Final
    static void print_summary(print_options = print_options::all);

    // Last executed test
    static void print_last();
  };

}

#endif
// NES_TEST__QTEST_H
