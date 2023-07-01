#pragma once

#include <bytes/serde.h>
#include <etl/array.h>
#include <etl/utility.h>
#include <mock/serial.h>
#include <nb/lock.h>
#include <nb/serial.h>
#include <nb/stream.h>
#include <stddef.h>

namespace media::uhf {
    class UHFId {
        uint8_t value_;

      public:
        UHFId() = delete;
        UHFId(const UHFId &) = default;
        UHFId(UHFId &&) = default;
        UHFId &operator=(const UHFId &) = default;
        UHFId &operator=(UHFId &&) = default;

        UHFId(const uint8_t &value) : value_{value} {}

        bool operator==(const UHFId &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const UHFId &other) const {
            return value_ != other.value_;
        }

        uint8_t value() const {
            return value_;
        }

        etl::array<uint8_t, sizeof(uint8_t)> serialize() const {
            return bytes::serialize(value_);
        }
    };

    template <size_t N>
    class ModemCommand {
        nb::stream::FixedByteReader<1> prefix_;
        nb::stream::FixedByteReader<2> command_;
        nb::stream::FixedByteReader<N> payload_;
        nb::stream::FixedByteReader<2> terminator_;
    };

    template <size_t N>
    class ModemResponse {
        nb::stream::FixedByteReader<1> prefix_;
        nb::stream::FixedByteReader<2> command_;
        nb::stream::FixedByteReader<N> response_;
        nb::stream::FixedByteReader<2> terminator_;

      public:
        using StreamWriterItem = uint8_t;

        ModemResponse() = default;
        ModemResponse(const ModemResponse &) = default;
        ModemResponse(ModemResponse &&) = default;
        ModemResponse &operator=(const ModemResponse &) = default;
        ModemResponse &operator=(ModemResponse &&) = default;

        size_t writable_count() const {
            return nb::stream::writable_count(prefix_, command_, response_, terminator_);
        }

        bool write(uint8_t data) {
            return nb::stream::write(data, prefix_, command_, response_, terminator_);
        }

        bool is_closed() const {
            return nb::stream::is_closed(prefix_, command_, response_, terminator_);
        }

        inline const auto &prefix() {
            return prefix_;
        }

        inline const auto &command() const {
            return command_;
        }

        inline const auto &response() const {
            return response_;
        }

        inline const auto &terminator() const {
            return terminator_;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<ModemResponse<1>>);

    template <typename T>
    class GetSerialNumber {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        nb::stream::FixedByteReader<5> command_ =
            nb::stream::make_fixed_stream_reader('@', 'S', 'N', '\r', '\n');
        ModemResponse<2> response_;

      public:
        using ReadableStreamItem = uint8_t;

        GetSerialNumber() = delete;
        GetSerialNumber(const GetSerialNumber &) = delete;
        GetSerialNumber(GetSerialNumber &&) = default;
        GetSerialNumber &operator=(const GetSerialNumber &) = delete;
        GetSerialNumber &operator=(GetSerialNumber &&) = default;

        GetSerialNumber(nb::lock::Guard<nb::serial::Serial<T>> &&serial)
            : serial_{etl::move(serial)} {}

        void execute() {
            nb::stream::pipe(command_, serial_);
            nb::stream::pipe(serial_, response_);
        }
    };

    static_assert(nb::stream::is_stream_reader_v<GetSerialNumber<mock::MockSerial>, uint8_t>);

    template <typename T>
    class ModemCommunicator {
        nb::lock::Lock<nb::serial::Serial<T>> serial_;

      public:
        ModemCommunicator() = delete;
        ModemCommunicator(const ModemCommunicator &) = delete;
        ModemCommunicator(ModemCommunicator &&) = default;
        ModemCommunicator &operator=(const ModemCommunicator &) = delete;
        ModemCommunicator &operator=(ModemCommunicator &&) = default;

        ModemCommunicator(nb::serial::Serial<T> &&serial) : serial_{etl::move(serial)} {}

        void get_serial_number() {}

        void set_equipment_id(const UHFId &id) {}

        void send_packet(const UHFId &destination, nb::stream::HeapStreamReader<uint8_t> &&stream) {
        }

        void receive_packet() {}
    };
} // namespace media::uhf
