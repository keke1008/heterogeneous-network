#include <doctest.h>

#include <nb/stream/fixed.h>
#include <nb/stream/single_line.h>
#include <util/u8_literal.h>

using namespace nb::stream;
using namespace util::u8_literal;

TEST_CASE("empty buffer must return pending") {
    SingleLineWritableBuffer<3> buffer;
    auto poll = buffer.poll();
    CHECK(poll.is_pending());
}

TEST_CASE("buffer with \\r\\n must return ready") {
    SingleLineWritableBuffer<3> buffer;
    nb::stream::FixedReadableBuffer<2> source{'\r', '\n'};
    auto poll = buffer.write_all_from(source);
    CHECK(poll.is_ready());
}

TEST_CASE("buffer with \\r must return pending") {
    SingleLineWritableBuffer<3> buffer;
    nb::stream::FixedReadableBuffer<1> source{'\r'};
    auto poll = buffer.write_all_from(source);
    CHECK(poll.is_pending());
}

TEST_CASE("buffer with \\n must return pending") {
    SingleLineWritableBuffer<3> buffer;
    nb::stream::FixedReadableBuffer<1> source{'\n'};
    auto poll = buffer.write_all_from(source);
    CHECK(poll.is_pending());
}

TEST_CASE("buffer contains \\r\\n must return ready") {
    SingleLineWritableBuffer<14> buffer;
    nb::stream::FixedReadableBuffer<14> source{"HELLO\r\nWORLD\r\n"_u8array};
    buffer.write_all_from(source);
    auto poll = buffer.poll();
    CHECK(poll.is_ready());
    CHECK(etl::equal(poll.unwrap(), etl::span("HELLO\r\n"_u8array)));
}
