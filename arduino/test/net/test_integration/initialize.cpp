#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/serial.h>
#include <net/app.h>
#include <util/rand.h>
#include <util/time.h>

TEST_CASE("initialize") {
    util::MockTime mock_time{0};
    util::MockRandom mock_random{0};
    net::App app{mock_time};

    mock::MockSerial serial;
    memory::Static<nb::stream::SerialStream<mock::MockSerial>> serial_stream{serial};
    app.add_serial_port(mock_time, serial_stream);

    for (uint8_t i = 0; i < 10; i++) {
        mock_time.advance(util::Duration::from_millis(150));
        app.execute(mock_time, mock_random);
    }
}
