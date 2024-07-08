#ifndef NES__NES_EXC_H
#define NES__NES_EXC_H

#include <format>
#include <stdexcept>

namespace nes {

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

#endif
// NES__NES_EXC_H
