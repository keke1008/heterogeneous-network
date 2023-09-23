#include <doctest.h>

#include <mock/stream.h>
#include <net/link/wifi/control/connect_to_ap.h>

using namespace net::link::wifi;
using namespace nb::stream;

TEST_CASE("ConnectToAp") {
    etl::array ssid = "ssid"_u8array;
    etl::array password = "password"_u8array;
    auto [future, promise] = nb::make_future_promise_pair<bool>();
    ConnectToAp connect_to_ap{etl::move(promise), ssid, password};

    mock::MockReadableWritableStream stream;

    SUBCASE("success response must return true") {
        etl::array response{"WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n"_u8array};
        stream.write_to_read_buffer(response);

        auto result = connect_to_ap.execute(stream);
        CHECK(result.is_ready());
        CHECK(future.poll().is_ready());
        CHECK(future.poll().unwrap());

        CHECK(stream.consume_write_buffer_and_equals_to( // フォーマッタの抑制
            R"(AT+CWJAP="ssid","password")"
            "\r\n"_u8array
        ));
    }

    SUBCASE("failure response must return false") {
        etl::array response{"\r\n\r\nFAIL\r\n"_u8array};
        stream.write_to_read_buffer(response);

        auto result = connect_to_ap.execute(stream);
        CHECK(result.is_ready());
        CHECK(future.poll().is_ready());
    }
}
