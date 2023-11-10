#include <doctest.h>
#include <util/doctest_ext.h>

#include <nb/stream/fixed.h>
#include <nb/stream/single_line.h>

using namespace nb::stream;

TEST_CASE("SingleLineWritableBuffer") {
    SUBCASE("empty buffer must return pending") {
        SingleLineWritableBuffer<3> buffer;
        auto poll = buffer.poll();
        CHECK(poll.is_pending());
    }

    SUBCASE("buffer with \\r\\n must return ready") {
        SingleLineWritableBuffer<3> buffer;
        nb::stream::FixedReadableBuffer<2> source{'\r', '\n'};
        auto poll = buffer.write_all_from(source);
        CHECK(poll.is_ready());
    }

    SUBCASE("buffer with \\r must return pending") {
        SingleLineWritableBuffer<3> buffer;
        nb::stream::FixedReadableBuffer<1> source{'\r'};
        auto poll = buffer.write_all_from(source);
        CHECK(poll.is_pending());
    }

    SUBCASE("buffer with \\n must return pending") {
        SingleLineWritableBuffer<3> buffer;
        nb::stream::FixedReadableBuffer<1> source{'\n'};
        auto poll = buffer.write_all_from(source);
        CHECK(poll.is_pending());
    }

    SUBCASE("buffer contains \\r\\n must return ready") {
        SingleLineWritableBuffer<14> buffer;
        nb::stream::FixedReadableBuffer<14> source{"HELLO\r\nWORLD\r\n"};
        buffer.write_all_from(source);
        auto poll = buffer.poll();
        CHECK(poll.is_ready());
        CHECK(util::as_str(poll.unwrap()) == "HELLO\r\n");
    }
}

TEST_CASE("MaxLengthSingleLineWritableBuffer") {
    SUBCASE("empty buffer must return pending") {
        MaxLengthSingleLineWrtableBuffer<3> buffer;
        CHECK(!buffer.written_bytes().has_value());
    }

    SUBCASE("buffer with \\r\\n must return ready") {
        MaxLengthSingleLineWrtableBuffer<3> buffer;
        nb::stream::FixedReadableBuffer<2> source{'\r', '\n'};
        auto poll = buffer.write_all_from(source);
        CHECK(poll.is_ready());
    }

    SUBCASE("buffer with \\r must return pending") {
        MaxLengthSingleLineWrtableBuffer<3> buffer;
        nb::stream::FixedReadableBuffer<1> source{'\r'};
        auto poll = buffer.write_all_from(source);
        CHECK(poll.is_pending());
    }

    SUBCASE("buffer with \\n must return pending") {
        MaxLengthSingleLineWrtableBuffer<3> buffer;
        nb::stream::FixedReadableBuffer<1> source{'\n'};
        auto poll = buffer.write_all_from(source);
        CHECK(poll.is_pending());
    }

    SUBCASE("buffer contains \\r\\n must return ready") {
        MaxLengthSingleLineWrtableBuffer<7> buffer;
        nb::stream::FixedReadableBuffer<14> source{"HELLO\r\nWORLD\r\n"};
        CHECK(buffer.write_all_from(source).is_ready());
        auto opt = buffer.written_bytes();
        CHECK(opt.has_value());
        CHECK(util::as_str(opt.value()) == "HELLO\r\n");
    }

    SUBCASE("long buffer") {
        MaxLengthSingleLineWrtableBuffer<5> buffer;
        nb::stream::FixedReadableBuffer<14> source{"HELLO\r\nFOO\r\n"};

        CHECK(buffer.write_all_from(source).is_ready());
        CHECK(!buffer.written_bytes().has_value());
        buffer.reset();

        CHECK(buffer.write_all_from(source).is_ready());
        auto opt = buffer.written_bytes();
        CHECK(opt.has_value());
        CHECK(util::as_str(opt.value()) == "FOO\r\n");
    }
}
