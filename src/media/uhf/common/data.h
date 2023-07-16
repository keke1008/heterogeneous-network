#pragma once

#include <memory/pair_shared.h>
#include <nb/future.h>
#include <nb/lock.h>

namespace media::uhf::common {
    template <typename Writer>
    class DataWriter {
        memory::Reference<nb::lock::Guard<Writer>> writer_;
        size_t writable_count_;

      public:
        DataWriter() = delete;
        DataWriter(const DataWriter &) = delete;
        DataWriter(DataWriter &&) = default;
        DataWriter &operator=(const DataWriter &) = delete;
        DataWriter &operator=(DataWriter &&) = default;

        DataWriter(memory::Reference<nb::lock::Guard<Writer>> &&writer, size_t length)
            : writer_{etl::move(writer)},
              writable_count_{length} {}

        inline constexpr bool is_writable() const {
            if (writer_.has_pair()) {
                return writer_.value()->is_writable();
            }
            return false;
        }

        inline constexpr auto writable_count() const {
            if (writer_.has_pair()) {
                return writer_.value()->writable_count();
            }
            return 0;
        }

        inline constexpr bool write(const uint8_t data) {
            if (!writer_.has_pair()) {
                return false;
            }
            if (!writer_.get()->get()->write(data)) {
                return false;
            }

            writable_count_--;
            if (writable_count_ == 0) {
                writer_.unpair();
            }
            return true;
        }

        inline constexpr bool is_writer_closed() const {
            if (writer_.has_pair()) {
                return writer_.value()->is_closed();
            }
            return true;
        }

        inline constexpr void close() {
            writer_.unpair();
        }
    };

    template <typename Reader>
    class DataReader {
        memory::Reference<nb::lock::Guard<Reader>> reader_;
        size_t readable_count_;
        nb::Promise<uint8_t> remain_bytes_promise_;

      public:
        DataReader() = delete;
        DataReader(const DataReader &) = delete;
        DataReader(DataReader &&) = default;
        DataReader &operator=(const DataReader &) = delete;
        DataReader &operator=(DataReader &&) = default;

        DataReader(
            memory::Reference<nb::lock::Guard<Reader>> &&reader,
            size_t length,
            nb::Promise<uint8_t> &&remain_bytes_promise
        )
            : reader_{etl::move(reader)},
              readable_count_{length},
              remain_bytes_promise_{etl::move(remain_bytes_promise)} {}

        ~DataReader() {
            remain_bytes_promise_.set_value(readable_count_);
        }

        inline constexpr bool is_readable() const {
            if (reader_.has_pair()) {
                return reader_.value()->is_readable();
            }
            return false;
        }

        inline constexpr auto readable_count() const {
            if (reader_.has_pair()) {
                return reader_.value()->readable_count();
            }
            return 0;
        }

        inline constexpr etl::optional<uint8_t> read() {
            if (!reader_.has_pair()) {
                return etl::nullopt;
            }

            const auto result = reader_.get()->get()->read();
            if (result.has_value()) {
                readable_count_--;
                if (readable_count_ == 0) {
                    reader_.unpair();
                    remain_bytes_promise_.set_value(0);
                }
            }
            return result;
        }

        inline constexpr bool is_reader_closed() const {
            if (reader_.has_pair()) {
                return reader_.get()->get()->is_reader_closed();
            }
            return true;
        }
    };
} // namespace media::uhf::common
