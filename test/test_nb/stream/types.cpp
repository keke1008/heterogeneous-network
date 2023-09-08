#include <doctest.h>

#include <nb/stream.h>
#include <vector>

using namespace nb::stream;

struct TestStreamReader final : StreamReader {
    std::vector<uint8_t> data_;

    template <typename... Ts>
    TestStreamReader(Ts... ts) : data_{static_cast<uint8_t>(ts)...} {}

    nb::Poll<Item> read() override {
        if (data_.empty()) {
            return nb::pending;
        }
        auto data = data_.front();
        data_.erase(data_.begin());
        return data;
    }

    nb::Poll<void> wait_until_empty() override {
        if (data_.empty()) {
            return nb::ready();
        }
        return nb::pending;
    }
};

struct TestStreamWriter final : StreamWriter<uint8_t> {
    std::vector<uint8_t> data_;

    nb::Poll<void> wait_until_writable() override {
        return nb::ready();
    }

    void write(Item item) override {
        data_.push_back(item);
    }

    nb::Poll<uint8_t> poll() override {
        return 1;
    }
};

struct TestFixedStreamWriter final : StreamWriter<uint8_t> {
    std::vector<uint8_t> data_;

    nb::Poll<void> wait_until_writable() override {
        if (data_.size() >= 3) {
            return nb::pending;
        }
        return nb::ready();
    }

    void write(Item item) override {
        data_.push_back(item);
    }

    nb::Poll<uint8_t> poll() override {
        return 1;
    }
};

TEST_CASE("Reader") {
    SUBCASE("empty") {
        TestStreamReader reader{};
        CHECK_EQ(reader.read(), nb::pending);
    }

    SUBCASE("single") {
        TestStreamReader reader{42};
        CHECK_EQ(reader.read(), 42);
        CHECK_EQ(reader.read(), nb::pending);
    }

    SUBCASE("multiple") {
        TestStreamReader reader{42, 43, 44};
        CHECK_EQ(reader.read(), 42);
        CHECK_EQ(reader.read(), 43);
        CHECK_EQ(reader.read(), 44);
        CHECK_EQ(reader.read(), nb::pending);
    }

    SUBCASE("fill writer") {
        TestStreamReader reader{42, 43, 44, 45, 46, 47};
        TestFixedStreamWriter writer1{};
        TestFixedStreamWriter writer2{};
        reader.fill_all(writer1, writer2);
        CHECK_EQ(writer1.data_, std::vector<uint8_t>{42, 43, 44});
        CHECK_EQ(writer2.data_, std::vector<uint8_t>{45, 46, 47});
    }
}

TEST_CASE("Writer") {
    TestStreamWriter writer{};

    SUBCASE("empt reader") {
        TestStreamReader reader{};
        writer.drain_all(reader);
        CHECK_EQ(writer.data_, std::vector<uint8_t>{});
    }

    SUBCASE("single reader") {
        TestStreamReader reader{42};
        writer.drain_all(reader);
        CHECK_EQ(writer.data_, std::vector<uint8_t>{42});
    }

    SUBCASE("multiple readers") {
        TestStreamReader reader1{1, 2, 3};
        TestStreamReader reader2{11, 12, 13};
        writer.drain_all(reader1, reader2);
        CHECK_EQ(writer.data_, std::vector<uint8_t>{1, 2, 3, 11, 12, 13});
    }
}
