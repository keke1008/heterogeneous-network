#include <doctest.h>

#include <mock/stream.h>
#include <net/link/serial.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;
using namespace net::link::serial;
using namespace net::link;

TEST_CASE("Executor") {
    mock::MockReadableWritableStream stream;
    net::link::SerialAddress address{0x12};
    SerialExecutor executor{stream, address};
    Address dest{SerialAddress{0x34}};

    SUBCASE("send_data") {
        frame::BodyLength length = 5;
        auto poll = executor.send_data(dest, length);
        CHECK(poll.is_ready());
        auto transmission = etl::move(poll.unwrap());
        CHECK(transmission.body.poll().is_pending());
        CHECK(transmission.success.poll().is_pending());

        executor.execute();
        CHECK(stream.consume_write_buffer_and_equals_to("\x12\x05"));
        CHECK(transmission.body.poll().is_ready());
        CHECK(transmission.success.poll().is_pending());

        auto writer = etl::move(transmission.body.poll().unwrap().get());
        CHECK(writer.writable_count() == length);
        writer.write("abcde"_u8array);
        writer.close();
        executor.execute();
        CHECK(stream.consume_write_buffer_and_equals_to("abcde"));
        CHECK(transmission.success.poll().is_ready());
        CHECK(transmission.success.poll().unwrap().get());
    }

    SUBCASE("receive data") {
        CHECK(executor.execute().is_pending());

        stream.read_buffer_.write("\x12\x05"_u8array);
        auto poll = executor.execute();
        CHECK(poll.is_ready());
        CHECK(poll.unwrap().body.poll().is_ready());
        CHECK(poll.unwrap().source.poll().is_ready());
        CHECK(poll.unwrap().source.poll().unwrap().get() == Address{address});

        auto reader = etl::move(poll.unwrap().body.poll().unwrap().get());
        CHECK(reader.total_length() == 5);
        CHECK(reader.readable_count() == 0);

        stream.read_buffer_.write("abcde"_u8array);
        nb::stream::FixedWritableBuffer<5> buffer;
        buffer.write_all_from(reader);
        CHECK(reader.readable_count() == 0);
        CHECK(buffer.writable_count() == 0);
        CHECK(etl::equal(buffer.written_bytes(), etl::span("abcde"_u8array)));
    }
}
