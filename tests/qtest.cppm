export module qtest;

import std;

// Declaration
export namespace nes::test {

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

// Implementation
using namespace std;
using namespace std::chrono;
using namespace std::filesystem;
namespace rng = std::ranges;
using namespace nes;
using namespace nes::test;

// qtest_exec_var
struct qtest_exec_var
{
    // Auxiliary global static variables 
    static string package_name_aux;
    static string sub_package_name_aux;
    static string sub_package_title_aux;
};

string qtest_exec_var::package_name_aux;
string qtest_exec_var::sub_package_name_aux;
string qtest_exec_var::sub_package_title_aux;

// read_line
string read_line(const source_location& loc)
{
    struct read_line_cache_t
    {
      string name;
      ifstream in;
      uint_least32_t line;
    };
    static vector<read_line_cache_t> cache;

    auto it = rng::find(cache, loc.file_name(), &read_line_cache_t::name);
    if (it != cache.end())
    {
      if (!it->in)
      {
        it->in.clear();
        it->in.seekg(0);
        it->line = 1;
      }
      else if (loc.line() < it->line)
      {
        it->in.seekg(0);
        it->line = 1;
      }
    }
    else
    {
      string source_file = loc.file_name();
      if (!path { loc.file_name() }.is_absolute())
        source_file = "../" + source_file;

      ifstream in { source_file };
      if (!in)
        return string { "Cannot open source file '" } + source_file + "'!";

      it = cache.insert(cache.end(), read_line_cache_t { loc.file_name(), move(in), 1 });
    }

    string line;
    for (uint_least32_t i = it->line; i <= loc.line() && getline(it->in, line); i++)
    {
      it->line++;
      if (i == loc.line())
        return line;
    }

    return string { "Cannot read line " } + to_string(loc.line()) +
                    "of source file '" + loc.file_name() + "'!";
}

// test_exec_t
class test_exec_t
{
    test_exec_status m_status;
    string m_test_str;
    source_location m_loc;
    system_clock::time_point m_tp { system_clock::now() };

  public:
    test_exec_t(test_exec_status status, string test_str, source_location loc)
      : m_status { status }
      , m_test_str { move(test_str) }
      , m_loc { move(loc) } {};

    test_exec_status status() const { return m_status; }
    bool error() const { return m_status != test_exec_status::ok; }

    string loc_str() const
    {
      return string { m_loc.file_name() } + ":" + to_string(m_loc.line());
    }

    string str() const
    {
      string ret { "[" };

      switch (m_status)
      {
        case test_exec_status::ok:  ret += " OK "; break;
        case test_exec_status::err: ret += " ER "; break;
        case test_exec_status::exc: ret += " EX "; break;
        default:                    ret += " UN "; break;
      }

      return ret + string { "] " } + read_line(m_loc) +
             (m_status == test_exec_status::ok ?
               string {} : string { "\n" }  +
               this->loc_str() + "\n" +
               m_test_str);
    }
};

// sub_package_title_t
class sub_package_title_t
{
    string m_title;
    vector<test_exec_t> m_execs;

  public:
    explicit sub_package_title_t(string title) : m_title { move(title) } {}

    const string& title() const { return m_title; }
    size_t size() const { return m_execs.size(); }
    size_t size_errors() const { return count_if(m_execs.begin(), m_execs.end(),
                                                 [](const auto& e) { return e.error(); }); }

    void exec_add(test_exec_status status, string test_str, source_location loc)
    {
      m_execs.push_back(test_exec_t { status, move(test_str), move(loc) });
    };

    bool empty_summary(print_options po) const
    {
      if (po == print_options::all)
        return !m_execs.size();
      else
        return rng::all_of(m_execs, [](const auto& e) { return e.status() == test_exec_status::ok; });
    }

    void print_summary(print_options po) const
    {
      cout << "Test " << m_title << "\n\n";

      int max_w = static_cast<int>(log10(m_execs.size())) + 1;
      for (size_t i = 0; i < m_execs.size(); i++) {
        if (po == print_options::all)
          cout << setw(max_w) << i + 1 << ": " << m_execs[i].str() << "\n";
        else if (po == print_options::only_errors && m_execs[i].status() != test_exec_status::ok)
          cout << setw(max_w) << i + 1 << ": " << m_execs[i].str() << "\n";
      }
      cout << '\n';
    }

    void print_last() const
    {
      if (!m_title.empty())
      cout << "Test " << m_title << "\n";

      if (!m_execs.empty())
        cout << "Localization: " << m_execs.back().loc_str() << "\n";
      else
        cout << "No test executed\n";
    }
};

// sub_package_t
class sub_package_t
{
    string m_name;
    vector<sub_package_title_t> m_sub_pkgs_titles;

  public:
    explicit sub_package_t(string name) : m_name { move(name) } {};

    const string& name() const { return m_name; }
    size_t size() const { return accumulate(m_sub_pkgs_titles.begin(), m_sub_pkgs_titles.end(), size_t { 0 },
                                            [](auto s, const auto& e) { return s + e.size(); }); }
    size_t size_errors() const { return accumulate(m_sub_pkgs_titles.begin(), m_sub_pkgs_titles.end(), size_t { 0 },
                                            [](auto s, const auto& e) { return s + e.size_errors(); }); }

    // Adiciona novo teste
    void exec_add(test_exec_status status, string test_str, source_location loc)
    {
      auto it = rng::find(m_sub_pkgs_titles, qtest_exec_var::sub_package_title_aux, &sub_package_title_t::title);
      if (it == m_sub_pkgs_titles.end())
        it = m_sub_pkgs_titles.insert(it, sub_package_title_t { qtest_exec_var::sub_package_title_aux });

      it->exec_add(status, move(test_str), move(loc));
    }

    // Emite o relatório do teste
    bool empty_summary(print_options po) const
    {
      return rng::all_of(m_sub_pkgs_titles, [&](const auto& e) { return e.empty_summary(po); });
    }

    void print_summary(print_options po) const
    {
      // Verifica se existe sumário a mostrar
      if (!this->empty_summary(po))
      {
        if (!m_name.empty())
          cout << "### Test " << m_name << " ###\n\n";

        for (const auto& spt : m_sub_pkgs_titles)
          if(!spt.empty_summary(po))
            spt.print_summary(po);
      }
    }

    void print_last() const
    {
      if (!m_name.empty())
        cout << "### Test " << m_name << " ###\n";

      if (!m_sub_pkgs_titles.empty())
        m_sub_pkgs_titles.back().print_last();
      else
        cout << "no subtitle executed\n";
    }
};

// package_t
class package_t
{
    string m_name;
    vector<sub_package_t> m_sub_pkgs;

  public:
    explicit package_t(string name) : m_name { move(name) } {}

    const string& name() const { return m_name; }
    size_t size() const { return accumulate(m_sub_pkgs.begin(), m_sub_pkgs.end(), size_t { 0 },
                                            [](auto s, const auto& e) { return s + e.size(); }); }
    size_t size_errors() const { return accumulate(m_sub_pkgs.begin(), m_sub_pkgs.end(), size_t { 0 },
                                            [](auto s, const auto& e) { return s + e.size_errors(); }); }

    // Adiciona novo teste
    void exec_add(test_exec_status status, string test_str, source_location loc)
    {
      auto it = rng::find(m_sub_pkgs, qtest_exec_var::sub_package_name_aux, &sub_package_t::name);
      if (it == m_sub_pkgs.end())
        it = m_sub_pkgs.insert(it, sub_package_t { qtest_exec_var::sub_package_name_aux });

      it->exec_add(status, move(test_str), move(loc));
    }

    // Emite o relatório do teste
    bool empty_summary(print_options po) const
    {
      return rng::all_of(m_sub_pkgs, [&](const auto& e) { return e.empty_summary(po); });
    }

    void print_summary(print_options po) const
    {
      // Verifica se existe sumário a mostrar
      if (!this->empty_summary(po))
      {
        if (!m_name.empty())
          cout << "--- Package " << m_name << " ---\n\n";

        for (const auto& sp : m_sub_pkgs)
          if (!sp.empty_summary(po))
            sp.print_summary(po);
      }
    }

    void print_last() const
    {
      if (!m_name.empty())
        cout << "--- Package " << m_name << " ---\n";

      if (!m_sub_pkgs.empty())
        m_sub_pkgs.back().print_last();
      else
        cout << "no subpackage executed\n";
    }
};

// qtest_exec
class qtest_exec
{
    vector<package_t> m_pkgs;

    qtest_exec() = default;

    static qtest_exec& inst()
    {
      static qtest_exec exec;

      return exec;
    };

  public:

    // Adiciona novo teste
    static void exec_add(test_exec_status status, string test_str, source_location loc)
    {
      auto& i = inst();
      auto it = rng::find(i.m_pkgs, qtest_exec_var::package_name_aux, &package_t::name);
      if (it == i.m_pkgs.end())
        it = i.m_pkgs.insert(it, package_t { qtest_exec_var::package_name_aux });

      it->exec_add(status, move(test_str), move(loc));
    };

    // Emite o relatório do teste
    static void print_summary(print_options po)
    {
      auto& i = inst();
      cout << "### Test Summary ###\n";

      for (const auto& pkg : i.m_pkgs)
        pkg.print_summary(po);

      auto num_testes = accumulate(i.m_pkgs.begin(), i.m_pkgs.end(), size_t { 0 },
                                   [](auto s, const auto& e) { return s + e.size(); });
      auto num_testes_errors = accumulate(i.m_pkgs.begin(), i.m_pkgs.end(), size_t { 0 },
                                   [](auto s, const auto& e) { return s + e.size_errors(); });

      cout << num_testes << " test executed in " << i.m_pkgs.size() << " package. ("
           << num_testes_errors << " errors).\n";
    };

    // Mostra qual foi o último teste realizado
    static void print_last()
    {
      auto& i = inst();
      cout << "### Last executed test ###\n";

      if (!i.m_pkgs.empty())
        i.m_pkgs.back().print_last();
      else
        cout << "no package executed\n";
    };
};

// Test API 
namespace nes::test {

  void qtest::exec_add(test_exec_status st, string test_str, source_location l)
  {
    qtest_exec::exec_add(st, move(test_str), move(l));
  }

  void qtest::package(string name)
  {
    qtest_exec_var::package_name_aux = move(name);
    qtest_exec_var::sub_package_name_aux.clear();
    qtest_exec_var::sub_package_title_aux.clear();
  }

  void qtest::sub_package(string name)
  {
    qtest_exec_var::sub_package_name_aux = move(name);
    qtest_exec_var::sub_package_title_aux.clear();
  }

  void qtest::sub_package_title(string title)
  {
    qtest_exec_var::sub_package_title_aux = move(title);
  }

  void qtest::print_summary(print_options po /*= print_options::all */)
  {
    qtest_exec::print_summary(po);
  }

  void qtest::print_last()
  {
    qtest_exec::print_last();
  }

}



