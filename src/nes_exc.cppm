export module nes_exc;

import std;

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
