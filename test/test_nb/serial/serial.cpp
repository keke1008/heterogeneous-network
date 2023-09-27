#include <doctest.h>

#include <mock/serial.h>
#include <nb/serial.h>
#include <nb/stream.h>

using namespace nb::stream;
using namespace nb::serial;

TEST_CASE("SerialStream") {
    mock::MockSerial mock_serial;
    nb::stream::SerialStream serial{mock_serial};

    SUBCASE("empty") {
        CHECK_EQ(serial.readable_count(), 0);
    }

    SUBCASE("read") {
        mock_serial.rx_buffer()->push_back(42);
        CHECK_EQ(serial.readable_count(), 1);
        CHECK_EQ(serial.read(), 42);
        CHECK_EQ(serial.readable_count(), 0);
    }

    SUBCASE("write") {
        FixedReadableBuffer<3>{42, 43, 44}.read_all_into(serial);
        CHECK_EQ(mock_serial.tx_buffer()->pop_front(), 42);
        CHECK_EQ(mock_serial.tx_buffer()->pop_front(), 43);
        CHECK_EQ(mock_serial.tx_buffer()->pop_front(), 44);
        CHECK(mock_serial.tx_buffer()->is_empty());
    }
}
