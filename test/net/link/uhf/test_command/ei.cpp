#include <doctest.h>

#include <mock/stream.h>
#include <net/link/uhf/command/ei.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("EI") {
    mock::MockReadableWritableStream stream{};

    SUBCASE("success") {
        net::link::uhf::ModemId equipment_id{0x12};
        net::link::uhf::EIExecutor executor{equipment_id};

        stream.write_to_read_buffer("*EI=12\r\n"_u8it);

        auto result = executor.poll(stream);
        CHECK(result.is_ready());

        for (auto ch : "@EI12\r\n"_u8it) {
            auto cmd = stream.write_buffer_.read();
            CHECK(cmd == ch);
        }
    }
}
