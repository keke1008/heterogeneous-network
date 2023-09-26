#include <doctest.h>

#include <mock/stream.h>
#include <net/link/uhf/executor.h>
#include <util/rand.h>
#include <util/u8_literal.h>

using namespace net::link::uhf;
using namespace util::u8_literal;

TEST_CASE("UHFExecutor") {
    util::MockTime time{0};
    util::MockRandom rand{50};

    mock::MockReadableWritableStream stream{};
    UHFExecutor uhf_executor{stream};

    SUBCASE("get_serial_number") {
        auto poll_fut = uhf_executor.get_serial_number();
        CHECK(poll_fut.is_ready());
        CHECK(poll_fut.unwrap().poll().is_pending());

        stream.write_to_read_buffer("*SN=123456789\r\n"_u8it);
        auto fut = etl::move(poll_fut.unwrap());
        CHECK(fut.poll().is_pending());

        uhf_executor.execute(time, rand);

        CHECK(fut.poll().is_ready());
        auto sn = etl::move(fut.poll().unwrap());
        CHECK(sn.get() == SerialNumber{"123456789"_u8array});
    }

    SUBCASE("set_equipment_id") {
        ModemId equipment_id{"AB"_u8array};

        auto poll_fut = uhf_executor.set_equipment_id(equipment_id);
        CHECK(poll_fut.is_ready());
        auto fut = etl::move(poll_fut.unwrap());

        stream.write_to_read_buffer("*EI=AB\r\n"_u8it);
        uhf_executor.execute(time, rand);
        CHECK(fut.poll().is_ready());
    }

    SUBCASE("data_transmission") {
        ModemId dest{"AB"_u8array};
        uint8_t length = 5;

        auto poll_fut = uhf_executor.send_data(net::link::Address{dest}, length);
        CHECK(poll_fut.is_ready());
        auto [f_body, f_result] = etl::move(poll_fut.unwrap());
        CHECK(f_body.poll().is_pending());
        CHECK(f_result.poll().is_pending());

        stream.write_to_read_buffer("*CS=EN\r\n*DT=05\r\n"_u8it);
        uhf_executor.execute(time, rand);
        CHECK(f_body.poll().is_ready());
        CHECK(f_result.poll().is_pending());

        auto &body = etl::move(f_body.poll().unwrap()).get();
        body.write("abcde"_u8array);
        body.close();
        uhf_executor.execute(time, rand);
        CHECK(f_result.poll().is_pending());

        time.set_now_ms(1000);
        uhf_executor.execute(time, rand);
        CHECK(f_result.poll().is_ready());

        auto result = etl::move(f_result.poll().unwrap());
        CHECK(result.get());
    }

    SUBCASE("data_receiving") {
        stream.write_to_read_buffer("*DR=05abcde\\RAB\r\n"_u8it);

        auto poll_fut = uhf_executor.execute(time, rand);
        CHECK(poll_fut.is_ready());
        auto fut = etl::move(poll_fut.unwrap());
        CHECK(fut.body.poll().is_ready());
        CHECK(fut.source.poll().is_pending());

        auto response = etl::move(fut.body.poll().unwrap());
        auto writer = nb::stream::FixedWritableBuffer<5>{};
        writer.write_all_from(response.get());
        response.get().close();

        CHECK(writer.poll().is_ready());
        CHECK(etl::equal(writer.poll().unwrap(), etl::span("abcde"_u8array)));

        uhf_executor.execute(time, rand);
        CHECK(fut.source.poll().is_ready());
        CHECK(fut.source.poll().unwrap().get() == net::link::Address{ModemId{"AB"_u8array}});
    }
}
