#include <doctest.h>
#include <util/doctest_ext.h>

#include <etl/span.h>
#include <mock/stream.h>
#include <net/link/uhf/command/cs.h>

TEST_CASE("CS") {
    mock::MockReadableWritableStream stream{};
    net::link::uhf::CSExecutor executor;

    SUBCASE("*CS=EN") {
        stream.read_buffer_.write_str("*CS=EN\r\n");

        auto result = executor.poll(stream);
        CHECK(util::as_str(stream.write_buffer_.written_bytes()) == "@CS\r\n");
        CHECK(result.is_ready());
        CHECK(result.unwrap());
    }

    SUBCASE("*CS=DI") {
        stream.read_buffer_.write_str("*CS=DI\r\n");
        auto result = executor.poll(stream);
        CHECK_FALSE(result.unwrap());
    }
}
