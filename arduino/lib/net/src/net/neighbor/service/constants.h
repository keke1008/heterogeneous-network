#pragma once

#include "../delay_socket.h"

namespace net::neighbor {
    constexpr NeighborSocketConfig SOCKET_CONFIG{
        .do_delay = false,
    };
}
