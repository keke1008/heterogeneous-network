#include <doctest.h>

#include <mock/serial.h>
#include <nb/serial.h>
#include <nb/stream.h>

using namespace nb::stream;
using namespace nb::serial;

TEST_CASE("SerialStream") {
    mock::MockSerial mock_serial;
    SerialStream serial{mock_serial};

    SUBCASE("empty") {
        CHECK(serial.read().is_pending());
    }

    SUBCASE("read") {
        mock_serial.rx_buffer()->push_back(42);
        CHECK_EQ(serial.read(), nb::ready<uint8_t>(42));
        CHECK(serial.read().is_pending());
    }

    SUBCASE("write") {
        serial.drain_all(FixedStreamReader<3>{42, 43, 44});
        CHECK_EQ(mock_serial.tx_buffer()->pop_front(), 42);
        CHECK_EQ(mock_serial.tx_buffer()->pop_front(), 43);
        CHECK_EQ(mock_serial.tx_buffer()->pop_front(), 44);
        CHECK(mock_serial.tx_buffer()->is_empty());
    }
}
