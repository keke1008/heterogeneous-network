#include "./initialization.h"

namespace net::link::wifi {
    const etl::array<etl::string_view, 3> COMMANDS = {
        "ATE0\r\n",          // Disable echo
        "AT+CWMODE=1\r\n",   // Set station mode
        "AT+CIPDINFO=1\r\n", // Show connection info
    };
}
