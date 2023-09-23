#include <doctest.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/link/uhf/command/dr.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("DT") {
    mock::MockReadableWritableStream stream{};
    auto [f, p] = nb::make_future_promise_pair<net::link::DataReader>();
    net::link::uhf::DRExecutor executor{etl::move(p)};

    SUBCASE("receive 'abc'") {
        stream.write_to_read_buffer("*DR=03abc\r\n"_u8it);

        auto poll = executor.poll(stream);
        CHECK(poll.is_pending());

        auto reader_poll = f.poll();
        CHECK(reader_poll.is_ready());

        auto &reader = reader_poll.unwrap().get();
        CHECK_EQ(reader.total_length(), 3);

        for (auto ch : "abc"_u8it) {
            CHECK_EQ(reader.read(), ch);
        }
        reader.close();

        auto result = executor.poll(stream);
        CHECK(result.is_ready());
    }
}
