export module net_exc;

import <stdexcept>;
import nes_exc;

export namespace nes::net {
  
  class net_exc : public nes::nes_exc
    { using nes_exc::nes_exc; };

  // Socket specific exceptions
  class socket_exc : public net_exc
    { using net_exc::net_exc; };

  class socket_disconnected : public socket_exc
    { using socket_exc::socket_exc; };

   class socket_timeout : public socket_exc
    { using socket_exc::socket_exc; };

   class socket_excess_data : public socket_exc
    { using socket_exc::socket_exc; };

}
