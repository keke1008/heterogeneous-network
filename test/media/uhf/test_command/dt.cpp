#include <doctest.h>

#include <media/uhf/command/dt.h>
#include <mock/serial.h>
#include <nb/future.h>
#include <nb/serial.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

using Serial = nb::serial::Serial<mock::MockSerial>;

TEST_CASE("DT") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    auto dest = media::uhf::ModemId{0x12};
    auto [f, p] = nb::make_future_promise_pair<media::uhf::CommandWriter<Serial>>();
    media::uhf::DTExecutor<Serial> executor{etl::move(serial), dest, 3, etl::move(p)};

    SUBCASE("send 'abc'") {
        for (auto ch : "*DT=03\r\n"_u8it) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        while (f.poll().is_pending()) {
            auto poll = executor.poll();
            CHECK(poll.is_pending());
        }

        auto &writer = f.poll().unwrap().get();
        for (auto ch : "abc"_u8it) {
            writer.write(ch);
        }
        writer.close();

        auto result = executor.poll();
        while (result.is_pending()) {
            result = executor.poll();
        }
        CHECK(result.unwrap());

        for (auto ch : "@DT03abc/R12\r\n"_u8it) {
            auto cmd = mock_serial.tx_buffer()->pop_front();
            CHECK(cmd.has_value());
            CHECK_EQ(cmd.value(), ch);
        }
    }
}
