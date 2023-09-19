#include <doctest.h>

#include "net/link/uhf/task/data_transmission.h"
#include <mock/stream.h>
#include <nb/lock.h>
#include <util/rand.h>
#include <util/u8_literal.h>

using namespace net::link::uhf;
using namespace net::link::uhf::data_transmisson;
using namespace util::u8_literal;

TEST_CASE("DataTransmissionTask") {
    SUBCASE("execute") {
        util::MockTime time{0};
        util::MockRandom rand{50};
        auto [f_result, p_result] = nb::make_future_promise_pair<bool>();
        auto [f, p] = nb::make_future_promise_pair<CommandWriter>();
        mock::MockReadableWritableStream stream{};

        DataTransmissionTask task{ModemId{0x12}, 3, etl::move(p), etl::move(p_result)};

        stream.write_to_read_buffer("*CS=DI\r\n*CS=EN\r\n*DT=03\r\n"_u8it);

        while (f.poll().is_pending()) {
            time.set_now(time.get_now() + 100);
            task.poll(stream, time, rand);
        }
        CHECK(f_result.poll().is_pending());

        auto writer = f.poll().unwrap();
        for (char ch : "abc"_u8it) {
            writer.get().write(ch);
        }
        writer.get().close();

        auto result = task.poll(stream, time, rand);
        while (result.is_pending()) {
            time.set_now(time.get_now() + 100);
            result = task.poll(stream, time, rand);
        }

        CHECK(result.is_ready());
        CHECK(f_result.poll().is_ready());
        CHECK(f_result.poll().unwrap());
        CHECK_EQ(stream.read_buffer_.readable_count(), 0);
    }
}
