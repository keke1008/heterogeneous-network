#include <doctest.h>

#include <etl/array.h>
#include <nb/stream/types.h>

using namespace nb::stream;

template <size_t N>
struct TestReadableBuffer final : public ReadableStream, public ReadableBuffer {
    etl::array<uint8_t, N> buffer_;
    uint8_t index_{0};

    TestReadableBuffer(const TestReadableBuffer &) = delete;
    TestReadableBuffer(TestReadableBuffer &&) = delete;
    TestReadableBuffer &operator=(const TestReadableBuffer &) = delete;
    TestReadableBuffer &operator=(TestReadableBuffer &&) = delete;

    template <typename... Ts>
    TestReadableBuffer(Ts... ts) : buffer_{static_cast<uint8_t>(ts)...} {}

    explicit TestReadableBuffer(etl::array<uint8_t, N> buffer) : buffer_(std::move(buffer)) {}

    uint8_t readable_count() const override {
        return buffer_.size() - index_;
    }

    uint8_t read() override {
        return buffer_[index_++];
    }

    nb::Poll<void> read_all_into(WritableStream &destination) override {
        bool continue_ = true;
        while (continue_ && readable_count() > 0 && destination.writable_count() > 0) {
            uint8_t write_count = etl::min(readable_count(), destination.writable_count());
            destination.write(etl::span(buffer_.data() + index_, write_count));
            index_ += write_count;
        }
        return readable_count() > 0 ? nb::pending : nb::ready();
    }
};

template <size_t N>
struct TestWritableBuffer final : public WritableStream, public WritableBuffer {
    etl::array<uint8_t, N> buffer_;
    uint8_t index_{0};

    TestWritableBuffer() {
        buffer_.fill(0);
    }

    uint8_t writable_count() const override {
        return buffer_.size() - index_;
    }

    bool write(uint8_t byte) override {
        if (writable_count() > 0) {
            buffer_[index_++] = byte;
        }
        return writable_count() > 0;
    }

    nb::Poll<void> write_all_from(ReadableStream &source) override {
        uint8_t read_count = etl::min(writable_count(), source.readable_count());
        source.read(etl::span(buffer_.data() + index_, read_count));
        index_ += read_count;
        return writable_count() > 0 ? nb::pending : nb::ready();
    }
};

TEST_CASE("write_all") {
    TestWritableBuffer<2> buffer1;
    TestWritableBuffer<2> buffer2;
    TestWritableBuffer<2> buffer3;

    SUBCASE("single WritableBuffer") {
        TestReadableBuffer<2> readable{42, 43};
        write_all_from(readable, buffer1);
        CHECK_EQ(buffer1.buffer_, etl::array<uint8_t, 2>{42, 43});
        CHECK_EQ(readable.readable_count(), 0);
    }

    SUBCASE("multiple WritableBuffer with same capacity") {
        TestReadableBuffer<6> readable{42, 43, 44, 45, 46, 47};
        write_all_from(readable, buffer1, buffer2, buffer3);
        CHECK_EQ(buffer1.buffer_, etl::array<uint8_t, 2>{42, 43});
        CHECK_EQ(buffer2.buffer_, etl::array<uint8_t, 2>{44, 45});
        CHECK_EQ(buffer3.buffer_, etl::array<uint8_t, 2>{46, 47});
        CHECK_EQ(readable.readable_count(), 0);
    }

    SUBCASE("multiple WritableBuffer with extra capacity") {
        TestReadableBuffer<3> readable{42, 43, 44};
        write_all_from(readable, buffer1, buffer2, buffer3);
        CHECK_EQ(buffer1.buffer_, etl::array<uint8_t, 2>{42, 43});
        CHECK_EQ(buffer2.buffer_, etl::array<uint8_t, 2>{44, 0});
        CHECK_EQ(buffer3.buffer_, etl::array<uint8_t, 2>{0, 0});
        CHECK_EQ(readable.readable_count(), 0);
    }

    SUBCASE("multiple WritableBuffer with less capacity") {
        TestReadableBuffer<7> readable{42, 43, 44, 45, 46, 47, 48};
        write_all_from(readable, buffer1, buffer2, buffer3);
        CHECK_EQ(buffer1.buffer_, etl::array<uint8_t, 2>{42, 43});
        CHECK_EQ(buffer2.buffer_, etl::array<uint8_t, 2>{44, 45});
        CHECK_EQ(buffer3.buffer_, etl::array<uint8_t, 2>{46, 47});
        CHECK_EQ(readable.readable_count(), 1);
    }
}

TEST_CASE("read_all") {
    TestReadableBuffer<2> buffer1{42, 43};
    TestReadableBuffer<2> buffer2{44, 45};
    TestReadableBuffer<2> buffer3{46, 47};

    SUBCASE("single ReadableBuffer") {
        TestWritableBuffer<2> write;
        read_all_into(write, buffer1);
        CHECK_EQ(write.buffer_, etl::array<uint8_t, 2>{42, 43});
        CHECK_EQ(buffer1.readable_count(), 0);
    }

    SUBCASE("multiple WritableBuffer with same capacity") {
        TestWritableBuffer<6> write;
        read_all_into(write, buffer1, buffer2, buffer3);
        CHECK_EQ(write.buffer_, etl::array<uint8_t, 6>{42, 43, 44, 45, 46, 47});
        CHECK_EQ(buffer1.readable_count(), 0);
        CHECK_EQ(buffer2.readable_count(), 0);
        CHECK_EQ(buffer3.readable_count(), 0);
    }

    SUBCASE("multiple WritableBuffer with extra capacity") {
        TestWritableBuffer<3> write;
        read_all_into(write, buffer1, buffer2, buffer3);
        CHECK_EQ(write.buffer_, etl::array<uint8_t, 3>{42, 43, 44});
        CHECK_EQ(buffer1.readable_count(), 0);
        CHECK_EQ(buffer2.readable_count(), 1);
        CHECK_EQ(buffer3.readable_count(), 2);
    }

    SUBCASE("multiple WritableBuffer with less capacity") {
        TestWritableBuffer<7> write;
        read_all_into(write, buffer1, buffer2, buffer3);
        CHECK_EQ(write.buffer_, etl::array<uint8_t, 7>{42, 43, 44, 45, 46, 47, 0});
        CHECK_EQ(buffer1.readable_count(), 0);
        CHECK_EQ(buffer2.readable_count(), 0);
        CHECK_EQ(buffer3.readable_count(), 0);
    }
}
