#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/uhf/command/ei.h>

TEST_CASE("EI") {
    mock::MockReadableWritableStream stream{};

    SUBCASE("success") {
        net::link::uhf::ModemId equipment_id{0x12};
        net::link::uhf::EIExecutor executor{equipment_id};

        stream.read_buffer_.write_str("*EI=12\r\n");

        auto result = executor.poll(stream);
        CHECK(result.is_ready());

        CHECK(util::as_str(stream.write_buffer_.written_bytes()) == "@EI12\r\n");
    }
}
