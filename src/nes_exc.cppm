#ifndef __clang__
module;

#include <format>
#endif
export module nes_exc;

#ifdef __clang__
import <format>;
#endif
import <stdexcept>;
import <string_view>;
import <string>;

export namespace nes {

  class nes_exc : public std::runtime_error
  { 

     public:
       template <class... Args>
       nes_exc(std::string_view msg_fmt, Args&&... args)
         : runtime_error { std::vformat(msg_fmt, std::make_format_args(args...)) }
       {
         
       }
  };

}
