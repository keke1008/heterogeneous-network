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

    SUBCASE("send_data") {
        frame::BodyLength length = 5;
        auto poll = executor.send_data(length);
        CHECK(poll.is_ready());
        auto future_writer = etl::move(poll.unwrap());
        CHECK(future_writer.poll().is_pending());

        executor.execute();
        CHECK(stream.consume_write_buffer_and_equals_to("\x12\x05"));
        CHECK(future_writer.poll().is_ready());

        auto writer = etl::move(future_writer.poll().unwrap().get());
        CHECK(writer.writable_count() == length);
        writer.write("abcde"_u8array);
        writer.close();
        executor.execute();
        CHECK(stream.consume_write_buffer_and_equals_to("abcde"));
    }

    SUBCASE("receive data") {
        CHECK(executor.execute().is_pending());

        stream.read_buffer_.write("\x12\x05"_u8array);
        auto poll = executor.execute();
        CHECK(poll.is_ready());
        CHECK(poll.unwrap().poll().is_ready());

        auto reader = etl::move(poll.unwrap().poll().unwrap().get());
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
