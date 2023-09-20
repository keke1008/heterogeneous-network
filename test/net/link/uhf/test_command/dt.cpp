#include <doctest.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/link/uhf/command/dt.h>
#include <util/time.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("DT") {
    mock::MockReadableWritableStream stream{};
    util::MockTime time{0};

    auto dest = net::link::uhf::ModemId{0x12};
    auto [f, p] = nb::make_future_promise_pair<net::link::uhf::CommandWriter>();
    net::link::uhf::DTExecutor executor{dest, 3, etl::move(p)};

    SUBCASE("send 'abc'") {
        stream.write_to_read_buffer("*DT=03\r\n"_u8it);

        auto poll = executor.poll(stream, time);
        CHECK(poll.is_pending());

        auto write_poll = f.poll();
        CHECK(write_poll.is_ready());

        auto &writer = write_poll.unwrap().get();
        CHECK_GE(writer.writable_count(), 3);
        for (auto ch : "abc"_u8it) {
            writer.write(ch);
        }
        writer.close();

        auto result = executor.poll(stream, time);
        while (result.is_pending()) {
            time.advance(util::Duration::from_millis(10));
            result = executor.poll(stream, time);
        }
        CHECK(result.unwrap());

        for (auto ch : "@DT03abc/R12\r\n"_u8it) {
            auto cmd = stream.write_buffer_.read();
            CHECK_EQ(cmd, ch);
        }
    }

    SUBCASE("receive information response") {
        stream.write_to_read_buffer("*DT=03\r\n"_u8it);
        stream.write_to_read_buffer("*IR=01\r\n"_u8it);

        auto poll = executor.poll(stream, time);
        CHECK(poll.is_pending());

        auto write_poll = f.poll();
        CHECK(write_poll.is_ready());

        auto &writer = write_poll.unwrap().get();
        for (auto ch : "abc"_u8it) {
            writer.write(ch);
        }
        writer.close();

        auto result = executor.poll(stream, time);
        while (result.is_pending()) {
            time.advance(util::Duration::from_millis(10));
            result = executor.poll(stream, time);
        }
        CHECK_FALSE(result.unwrap());
    }
}
