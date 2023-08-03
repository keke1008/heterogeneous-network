#include <doctest.h>

#include <media/uhf/modem.h>

TEST_CASE("Response") {
    media::uhf::modem::GetSerialNumberResponseBody response;
    auto input =
        nb::stream::TinyByteReader<11>{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', '\r', '\n'};

    auto result = response.read_from(input);
    CHECK(result.is_ready());
    CHECK(input.is_reader_closed());

    auto actual = result.unwrap();
    media::uhf::modem::SerialNumber expected{{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'}};
    CHECK_EQ(actual, expected);
}
