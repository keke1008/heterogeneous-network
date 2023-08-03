#pragma once

#include <memory/pair_shared.h>
#include <nb/future.h>

namespace media::uhf::common {
    template <typename Writer>
    class DataWriter {
        memory::Reference<Writer> writer_;

      public:
        DataWriter() = delete;
        DataWriter(const DataWriter &) = delete;
        DataWriter(DataWriter &&) = default;
        DataWriter &operator=(const DataWriter &) = delete;
        DataWriter &operator=(DataWriter &&) = default;

        explicit DataWriter(memory::Reference<Writer> &&writer) : writer_{etl::move(writer)} {}

        inline constexpr bool is_writable() const {
            if (writer_.has_pair()) {
                return writer_.get()->get().is_writable();
            }
            return false;
        }

        inline constexpr auto writable_count() const {
            if (writer_.has_pair()) {
                return writer_.get()->get().writable_count();
            }
            return 0;
        }

        inline constexpr bool write(const uint8_t data) {
            if (!writer_.has_pair()) {
                return false;
            }
            if (!writer_.get()->get().write(data)) {
                return false;
            }
            return true;
        }

        inline constexpr bool is_writer_closed() const {
            if (writer_.has_pair()) {
                return writer_.get()->get().is_closed();
            }
            return true;
        }

        inline constexpr void close() {
            writer_.unpair();
        }
    };

    template <typename Reader>
    class DataReader {
        memory::Reference<Reader> reader_;

      public:
        DataReader() = delete;
        DataReader(const DataReader &) = delete;
        DataReader(DataReader &&) = default;
        DataReader &operator=(const DataReader &) = delete;
        DataReader &operator=(DataReader &&) = default;

        DataReader(memory::Reference<Reader> &&reader) : reader_{etl::move(reader)} {}

        inline constexpr bool is_readable() const {
            if (reader_.has_pair()) {
                return reader_.get()->get().is_readable();
            }
            return false;
        }

        inline constexpr decltype(etl::declval<Reader>().readable_count()) readable_count() const {
            if (reader_.has_pair()) {
                return reader_.get()->get().readable_count();
            }
            return 0;
        }

        inline constexpr etl::optional<uint8_t> read() {
            if (!reader_.has_pair()) {
                return etl::nullopt;
            }

            const auto result = reader_.get()->get().read();
            if (result.has_value()) {}
            return result;
        }

        inline constexpr bool is_reader_closed() const {
            if (reader_.has_pair()) {
                return reader_.get()->get().is_reader_closed();
            }
            return true;
        }

        inline constexpr void close() {
            reader_.unpair();
        }
    };
} // namespace media::uhf::common
