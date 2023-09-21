#include <doctest.h>

#include <nb/stream/fixed.h>
#include <nb/stream/tuple.h>

using namespace nb::stream;

TEST_CASE("TupleReadableBuffer") {
    TupleReadableBuffer reader{
        FixedReadableBuffer<3>{0x01, 0x02, 0x03},
        FixedReadableBuffer<3>{0x04, 0x05, 0x06},
    };
    FixedWritableBuffer<6> writer;

    CHECK(reader.read_all_into(writer).is_ready());

    etl::array<uint8_t, 6> expected{0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    CHECK(etl::equal(writer.poll().unwrap(), etl::span(expected)));
}

TEST_CASE("TupleWritableBuffer") {
    TupleWritableBuffer writer{
        FixedWritableBuffer<3>{},
        FixedWritableBuffer<3>{},
    };
    FixedReadableBuffer<6> reader{0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    CHECK(writer.write_all_from(reader).is_ready());

    etl::array<uint8_t, 3> expected1{0x01, 0x02, 0x03};
    CHECK(etl::equal(writer.get<0>().poll().unwrap(), etl::span(expected1)));

    etl::array<uint8_t, 3> expected2{0x04, 0x05, 0x06};
    CHECK(etl::equal(writer.get<1>().poll().unwrap(), etl::span(expected2)));
}
