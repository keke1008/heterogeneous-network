#include <doctest.h>

#include <media/uhf/command/dr.h>
#include <mock/serial.h>
#include <nb/future.h>
#include <nb/serial.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

using Serial = nb::serial::Serial<mock::MockSerial>;

TEST_CASE("DT") {
    auto mock_serial = mock::MockSerial{};
    auto serial = memory::Owned{nb::serial::Serial{mock_serial}};

    auto [f, p] = nb::make_future_promise_pair<media::uhf::ResponseReader<Serial>>();
    media::uhf::DRExecutor<Serial> executor{etl::move(p)};

    SUBCASE("receive 'abc'") {
        for (auto ch : "*DR=03abc\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        while (f.poll().is_pending()) {
            auto poll = executor.poll(serial);
            CHECK(poll.is_pending());
        }

        auto &reader = f.poll().unwrap().get();
        CHECK_EQ(reader.total_length(), 3);
        for (auto ch : "abc"_u8it) {
            auto res = reader.read();
            CHECK(res.has_value());
            CHECK_EQ(res.value(), ch);
        }
        reader.close();

        auto result = executor.poll(serial);
        while (result.is_pending()) {
            result = executor.poll(serial);
        }
    }
}
