#include <doctest.h>

#include <etl/span.h>
#include <mock/stream.h>
#include <net/link/uhf/command/cs.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("CS") {
    mock::MockReadableWritableStream stream{};
    net::link::uhf::CSExecutor executor;

    SUBCASE("*CS=EN") {
        stream.write_to_read_buffer("*CS=EN\r\n"_u8it);

        auto result = executor.poll(stream);
        for (auto ch : "@CS\r\n"_u8it) {
            auto cmd = stream.write_buffer_.read();
            CHECK(cmd == ch);
        }

        CHECK(result.is_ready());
        CHECK(result.unwrap());
    }

    SUBCASE("*CS=DI") {
        stream.write_to_read_buffer("*CS=DI\r\n"_u8it);

        auto result = executor.poll(stream);

        CHECK_FALSE(result.unwrap());
    }
}
