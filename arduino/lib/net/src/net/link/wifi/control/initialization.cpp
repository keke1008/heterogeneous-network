#include "./initialization.h"

namespace net::link::wifi {
    const etl::array<etl::string_view, 3> COMMANDS = {
        R"(ATE0)",          // Disable echo
        R"(AT+CWMODE=1)",   // Set station mode
        R"(AT+CIPDINFO=1)", // Show connection info
    };
}
