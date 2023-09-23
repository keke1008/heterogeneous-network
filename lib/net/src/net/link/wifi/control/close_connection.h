#pragma once

#include "../link.h"
#include "./generic.h"
#include <nb/stream.h>

namespace net::link::wifi {
    class CloseConnection final
        : public Control<15, message::ResponseType::OK, message::ResponseType::ERROR> {
      public:
        explicit CloseConnection(nb::Promise<bool> &&promise, LinkId link_id)
            : Control{etl::move(promise), "AT+CIPCLOSE=", link_id_to_byte(link_id), "\r\n"} {}
    };
} // namespace net::link::wifi
