#include "./initialization.h"

namespace net::link::wifi {
    const etl::array<etl::string_view, 5> COMMANDS = {
        R"(ATE0)",             // Disable echo
        R"(AT+CWMODE=1)",      // Set station mode
        R"(AT+CIPMUX=1)",      // Enable multiple connections
        R"(AT+CIPDINFO=1)",    // Show connection info
        R"(AT+CIPRECVMODE=1)", // Enable passive receive mode
    };
}
