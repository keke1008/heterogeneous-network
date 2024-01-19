#include "./initialization.h"

namespace media::wifi {
    const etl::array<etl::string_view, 3> COMMANDS = {
        "AT+CIPMUX=0\r\n",   // Disable multiple connections
        "AT+CWMODE=1\r\n",   // Set station mode
        "AT+CIPDINFO=1\r\n", // Show connection info
    };
}
