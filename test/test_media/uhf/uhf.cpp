#include <doctest.h>

#include <media/uhf/uhf.h>

TEST_CASE("UHF") {
    using Serial = nb::serial::Serial<mock::MockSerial>;

    mock::MockSerial mock_serial;
    Serial serial{mock_serial};
    nb::lock::Lock<Serial> locked_serial{etl::move(serial)};
    auto tx_buffer = mock_serial.tx_buffer();
    auto rx_buffer = mock_serial.rx_buffer();
    media::uhf::UHF uhf{etl::move(locked_serial)};

    SUBCASE("get_serial_number") {
        auto future = uhf.get_serial_number();
        for (char ch : {'@', 'S', 'N', '\r', '\n'}) {
            auto actual = tx_buffer->pop_front();
            if (!actual.has_value()) {
                uhf.execute();
                continue;
            }
            CHECK_EQ(actual.value(), ch);
        }

        for (char ch : {'*', 'S', 'N', '=', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'}) {
            rx_buffer->push_back(ch);
        }

        while (future.get().is_pending()) {
            uhf.execute();
        }
        auto result = future.get().unwrap();
        media::uhf::modem::SerialNumber expected = {{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'}};
        CHECK_EQ(result, expected);
    }
}
