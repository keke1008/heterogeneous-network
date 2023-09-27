#include <doctest.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/link/uhf/command/dr.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;
using namespace net::link;

TEST_CASE("DT") {
    mock::MockReadableWritableStream stream{};
    auto [f_body, p_body] = nb::make_future_promise_pair<net::link::DataReader>();
    auto [f_source, p_source] = nb::make_future_promise_pair<net::link::Address>();
    net::link::uhf::DRExecutor executor{etl::move(p_body), etl::move(p_source)};

    SUBCASE("receive 'abc'") {
        stream.write_to_read_buffer("*DR=03abc\\RAB\r\n"_u8it);

        auto poll = executor.poll(stream);
        CHECK(poll.is_pending());

        auto reader_poll = f_body.poll();
        CHECK(reader_poll.is_ready());
        CHECK(f_source.poll().is_pending());

        auto &reader = reader_poll.unwrap().get();
        CHECK_EQ(reader.total_length(), 3);

        for (auto ch : "abc"_u8it) {
            CHECK_EQ(reader.read(), ch);
        }
        reader.close();

        auto result = executor.poll(stream);
        CHECK(result.is_ready());
        CHECK(f_source.poll().is_ready());
        uhf::ModemId expected_source{0xAB};
        CHECK_EQ(f_source.poll().unwrap().get(), Address{expected_source});
    }
}
