#include <doctest.h>
#include <util/doctest_ext.h>

#include <nb/serde.h>

template <uint8_t LENGTH>
struct ReadableWritable {
    etl::array<uint8_t, LENGTH> data{};
    uint8_t read_count{0};
    uint8_t written_count{0};

    nb::Poll<nb::de::DeserializeResult> poll_readable(uint8_t count) {
        uint8_t required_read_count = read_count + count;
        if (required_read_count > LENGTH) {
            return nb::de::DeserializeResult::NotEnoughLength;
        }

        if (required_read_count > written_count) {
            return nb::pending;
        }

        return nb::de::DeserializeResult::Ok;
    }

    uint8_t read_unchecked() {
        return data[read_count++];
    }

    nb::Poll<etl::pair<uint8_t, nb::de::DeserializeResult>> read() {
        auto result = POLL_UNWRAP_OR_RETURN(poll_readable(1));
        if (result != nb::de::DeserializeResult::Ok) {
            return etl::pair{static_cast<uint8_t>(0), result};
        }

        return etl::pair{read_unchecked(), nb::de::DeserializeResult::Ok};
    }

    nb::Poll<nb::ser::SerializeResult> poll_writable(uint8_t write_count) {
        uint8_t required_write_count = written_count + write_count;
        if (required_write_count > LENGTH) {
            return nb::ser::SerializeResult::NotEnoughLength;
        }

        return nb::ser::SerializeResult::Ok;
    }

    void write_unchecked(uint8_t byte) {
        data[written_count++] = byte;
    }

    nb::Poll<nb::ser::SerializeResult> write(uint8_t byte) {
        auto result = POLL_UNWRAP_OR_RETURN(poll_writable(1));
        if (result != nb::ser::SerializeResult::Ok) {
            return result;
        }

        write_unchecked(byte);
        return nb::ser::SerializeResult::Ok;
    }
};

static_assert(nb::de::AsyncReadable<ReadableWritable<1>>);
static_assert(nb::ser::AsyncWritable<ReadableWritable<1>>);

TEST_CASE("Bin") {
    SUBCASE("Just enough length") {
        ReadableWritable<4> buf{};
        uint32_t value = 0x12345678;

        nb::ser::Bin<uint32_t> ser{value};
        auto ser_poll_result = ser.serialize(buf);
        CHECK(ser_poll_result.is_ready());
        CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);
        CHECK(buf.written_count == 4);

        nb::de::Bin<uint32_t> de{};
        auto de_poll_result = de.deserialize(buf);
        CHECK(de_poll_result.is_ready());
        CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
        CHECK(de.result() == value);
    }

    SUBCASE("Not enough length") {
        ReadableWritable<3> buf{};
        uint32_t value = 0x12345678;

        nb::ser::Bin<uint32_t> ser{value};
        auto ser_poll_result = ser.serialize(buf);
        CHECK(ser_poll_result.is_ready());
        CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::NotEnoughLength);

        nb::de::Bin<uint32_t> de{};
        auto de_poll_result = de.deserialize(buf);
        CHECK(de_poll_result.is_ready());
        CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::NotEnoughLength);
        CHECK(de.result() == 0);
    }
}

TEST_CASE("Bool") {
    ReadableWritable<1> buf{};
    bool value = true;

    nb::ser::Bool ser{value};
    auto ser_poll_result = ser.serialize(buf);
    CHECK(ser_poll_result.is_ready());
    CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);

    nb::de::Bool de{};
    auto de_poll_result = de.deserialize(buf);
    CHECK(de_poll_result.is_ready());
    CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
    CHECK(de.result() == value);
}

TEST_CASE("Hex") {
    SUBCASE("Just enough length") {
        ReadableWritable<8> buf{};
        uint32_t value = 0x1A2B3C4D;

        nb::ser::Hex<uint32_t> ser{value};
        auto ser_poll_result = ser.serialize(buf);
        CHECK(ser_poll_result.is_ready());
        CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);
        CHECK(util::as_str(buf.data) == "1A2B3C4D");

        nb::de::Hex<uint32_t> de{};
        auto de_poll_result = de.deserialize(buf);
        CHECK(de_poll_result.is_ready());
        CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
        CHECK(de.result() == value);
    }

    SUBCASE("Not enough length") {
        ReadableWritable<7> buf{};
        uint32_t value = 0x1A2B3C4D;

        nb::ser::Hex<uint32_t> ser{value};
        auto ser_poll_result = ser.serialize(buf);
        CHECK(ser_poll_result.is_ready());
        CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::NotEnoughLength);

        nb::de::Hex<uint32_t> de{};
        auto de_poll_result = de.deserialize(buf);
        CHECK(de_poll_result.is_ready());
        CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::NotEnoughLength);
        CHECK(de.result() == 0);
    }
}

TEST_CASE("Dec") {
    ReadableWritable<4> buf{};
    uint16_t value = 1234;

    nb::ser::Dec<uint16_t> ser{value};
    auto ser_poll_result = ser.serialize(buf);
    CHECK(ser_poll_result.is_ready());
    CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);
    CHECK(util::as_str(buf.data) == "1234");

    nb::de::Dec<uint16_t> de{};
    auto de_poll_result = de.deserialize(buf);
    CHECK(de_poll_result.is_ready());
    CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
    CHECK(de.result() == value);
}

TEST_CASE("Optional") {
    SUBCASE("has value") {
        ReadableWritable<5> buf{};
        etl::optional<uint32_t> value = 0x12345678;

        nb::ser::Optional<nb::ser::Bin<uint32_t>> ser{value};
        auto ser_poll_result = ser.serialize(buf);
        CHECK(ser_poll_result.is_ready());
        CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);
        CHECK(buf.data[0] == 1);

        nb::de::Optional<nb::de::Bin<uint32_t>> de{};
        auto de_poll_result = de.deserialize(buf);
        CHECK(de_poll_result.is_ready());
        CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
        CHECK(de.result() == value);
    }

    SUBCASE("no value") {
        ReadableWritable<5> buf{};
        etl::optional<uint32_t> value{etl::nullopt};

        nb::ser::Optional<nb::ser::Bin<uint32_t>> ser{value};
        auto ser_poll_result = ser.serialize(buf);
        CHECK(ser_poll_result.is_ready());
        CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);
        CHECK(buf.data[0] == 0);

        nb::de::Optional<nb::de::Bin<uint32_t>> de{};
        auto de_poll_result = de.deserialize(buf);
        CHECK(de_poll_result.is_ready());
        CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
        CHECK(de.result() == value);
    }
}

TEST_CASE("Vec") {
    SUBCASE("Just enough length") {
        ReadableWritable<5> buf{};
        etl::array<uint8_t, 2> value{1, 2};

        nb::ser::Vec<nb::ser::Bin<uint8_t>, 3> ser{value};
        auto ser_poll_result = ser.serialize(buf);
        CHECK(ser_poll_result.is_ready());
        CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);
        CHECK(buf.data[0] == 2);

        nb::de::Vec<nb::de::Bin<uint8_t>, 3> de{};
        auto de_poll_result = de.deserialize(buf);
        CHECK(de_poll_result.is_ready());
        CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
        auto result = de.result();
        CHECK(result.size() == 2);
        for (uint8_t i = 0; i < 2; ++i) {
            CHECK(result[i] == value[i]);
        }
    }
}

TEST_CASE("composit") {
    ReadableWritable<10> buf{};
    etl::array<etl::optional<etl::array<uint16_t, 1>>, 2> value{
        etl::nullopt,
        etl::optional{etl::array<uint16_t, 1>{0x1234}},
    };

    nb::ser::Vec<nb::ser::Optional<nb::ser::Vec<nb::ser::Bin<uint16_t>, 1>>, 2> ser{value};
    auto ser_poll_result = ser.serialize(buf);
    CHECK(ser_poll_result.is_ready());
    CHECK(ser_poll_result.unwrap() == nb::ser::SerializeResult::Ok);

    nb::de::Vec<nb::de::Optional<nb::de::Vec<nb::de::Bin<uint16_t>, 1>>, 2> de{};
    auto de_poll_result = de.deserialize(buf);
    CHECK(de_poll_result.is_ready());
    CHECK(de_poll_result.unwrap() == nb::de::DeserializeResult::Ok);
    auto result = de.result();
    CHECK(result.size() == 2);
    CHECK(result[0] == etl::nullopt);
    CHECK(result[1].has_value());
    CHECK(result[1].value().size() == 1);
    CHECK(result[1].value()[0] == 0x1234);
}
