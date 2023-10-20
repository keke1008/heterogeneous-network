#include "serial.h"

namespace mock {
    etl::pair<MockSerial, MockSerial> make_connected_mock_serials() {
        memory::Shared buf1{memory::CircularBuffer<etl::array<uint8_t, 64>>::make_stack<64>()};
        memory::Shared buf2{memory::CircularBuffer<etl::array<uint8_t, 64>>::make_stack<64>()};
        MockSerial serial1{buf1, buf2};
        MockSerial serial2{buf2, buf1};
        return {etl::move(serial1), etl::move(serial2)};
    }
} // namespace mock
