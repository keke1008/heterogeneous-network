#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/wifi/control/join_ap.h>

using namespace net::link::wifi;
using namespace nb::stream;

TEST_CASE("JoinAp") {
    etl::array<uint8_t, 4> ssid{'s', 's', 'i', 'd'};
    etl::array<uint8_t, 8> password{'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    auto [future, promise] = nb::make_future_promise_pair<bool>();
    JoinAp join_ap{etl::move(promise), ssid, password};

    mock::MockReadableWritableStream stream;

    SUBCASE("success response must return true") {
        stream.read_buffer_.write_str("WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n");

        auto result = join_ap.execute(stream);
        CHECK(result.is_ready());
        CHECK(future.poll().is_ready());
        CHECK(future.poll().unwrap());

        CHECK(
            util::as_str(stream.write_buffer_.written_bytes()) ==
            "AT+CWJAP=\"ssid\",\"password\"\r\n"
        );
    }

    SUBCASE("failure response must return false") {
        stream.read_buffer_.write_str("\r\n\r\nFAIL\r\n");

        auto result = join_ap.execute(stream);
        CHECK(result.is_ready());
        CHECK(future.poll().is_ready());
    }
}
