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

  class qtest
  {
    static void exec_add(bool, std::string, std::source_location);

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
      qtest::exec_add(std::forward<T>(lhs) == std::forward<U>(rhs),
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void neq(T&& lhs, U&& rhs,
                    std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) != std::forward<U>(rhs),
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void lt(T&& lhs, U&& rhs,
                   std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) < std::forward<U>(rhs),
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void lteq(T&& lhs, U&& rhs,
                     std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) <= std::forward<U>(rhs),
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void gt(T&& lhs, U&& rhs,
                   std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) > std::forward<U>(rhs),
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    template <class T, class U>
    static void gteq(T&& lhs, U&& rhs,
                     std::source_location l = std::source_location::current())
    {
      qtest::exec_add(std::forward<T>(lhs) >= std::forward<U>(rhs),
                      detail::print_values_test(std::forward<T>(lhs), std::forward<U>(rhs)),
                      std::move(l));
    }

    // Boolean operations
    template <class T>
    static void is_true(T&& op,
                        std::source_location l = std::source_location::current())
    {
      qtest::exec_add(static_cast<bool>(std::forward<T>(op)),
                      detail::print_values_test(static_cast<bool>(std::forward<T>(op)), true), std::move(l));
    }

    template <class T>
    static void is_false(T&& op,
                         std::source_location l = std::source_location::current())
    {
      qtest::exec_add(!static_cast<bool>(std::forward<T>(op)),
                      detail::print_values_test(static_cast<bool>(std::forward<T>(op)), false), std::move(l));
    }

    // Flow operations
    static void unreachable(std::string msg = {},
                            std::source_location l = std::source_location::current())
    {
      qtest::exec_add(false, "unreachable" + msg, std::move(l));
    }

    static void ok(std::string msg,
                   std::source_location l = std::source_location::current())
    {
      qtest::exec_add(true, std::move(msg), std::move(l));
    }

    // Final
    static void print_summary(print_options = print_options::all);

    // Last executed test
    static void print_last();
  };

}

#endif
// NES_TEST__QTEST_H
