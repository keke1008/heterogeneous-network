#pragma once

#include <bytes/serde.h>
#include <etl/array.h>
#include <etl/optional.h>
#include <etl/utility.h>
#include <mock/serial.h>
#include <nb/lock.h>
#include <nb/serial.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <stddef.h>

namespace media::uhf::modem {
    class UHFId {
        etl::array<uint8_t, 2> value_;

      public:
        UHFId() = delete;
        UHFId(const UHFId &) = default;
        UHFId(UHFId &&) = default;
        UHFId &operator=(const UHFId &) = default;
        UHFId &operator=(UHFId &&) = default;

        UHFId(const etl::array<uint8_t, 2> &value) : value_{value} {}

        bool operator==(const UHFId &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const UHFId &other) const {
            return value_ != other.value_;
        }

        const etl::array<uint8_t, 2> &value() const {
            return value_;
        }
    };

    template <size_t N>
    class FixedCommand {
        nb::stream::FixedByteReader<1> prefix_{'@'};
        nb::stream::FixedByteReader<2> command_name_;
        nb::stream::FixedByteReader<N> value_;
        nb::stream::FixedByteReader<2> terminator_{'\r', '\n'};

      public:
        using StreamReaderItem = uint8_t;

        FixedCommand() = delete;
        FixedCommand(const FixedCommand &) = default;
        FixedCommand(FixedCommand &&) = default;
        FixedCommand &operator=(const FixedCommand &) = default;
        FixedCommand &operator=(FixedCommand &&) = default;

        FixedCommand(
            nb::stream::FixedByteReader<2> &&command_name,
            nb::stream::FixedByteReader<N> &&value
        )
            : command_name_{command_name},
              value_{value_} {}

        bool is_readable() const {
            return nb::stream::is_readable(prefix_, command_name_, value_, terminator_);
        }

        size_t readable_count() const {
            return nb::stream::readable_count(prefix_, command_name_, value_, terminator_);
        }

        etl::optional<uint8_t> read() {
            return nb::stream::read(prefix_, command_name_, value_, terminator_);
        }

        bool is_closed() const {
            return nb::stream::is_closed(prefix_, command_name_, value_, terminator_);
        }
    };

    static_assert(nb::stream::is_finite_stream_reader_v<FixedCommand<1>>);

    template <size_t N>
    class FixedResponse {
        nb::stream::FixedByteWriter<1> prefix_;
        nb::stream::FixedByteWriter<2> command_name_;
        nb::stream::FixedByteWriter<1> equal_;
        nb::stream::FixedByteWriter<N> value_;
        nb::stream::FixedByteWriter<2> terminator_;

      public:
        using StreamWriterItem = uint8_t;

        FixedResponse() = default;
        FixedResponse(const FixedResponse &) = default;
        FixedResponse(FixedResponse &&) = default;
        FixedResponse &operator=(const FixedResponse &) = default;
        FixedResponse &operator=(FixedResponse &&) = default;

        bool is_writable() const {
            return nb::stream::is_writable(prefix_, command_name_, equal_, value_, terminator_);
        }

        size_t writable_count() const {
            return nb::stream::writable_count(prefix_, command_name_, equal_, value_, terminator_);
        }

        bool write(uint8_t data) {
            return nb::stream::write(data, prefix_, command_name_, equal_, value_, terminator_);
        }

        bool is_closed() const {
            return nb::stream::is_closed(prefix_, command_name_, equal_, value_, terminator_);
        }

        inline const auto &prefix() const {
            return prefix_.get();
        }

        inline const auto &command() const {
            return command_name_.get();
        }

        inline const auto &response() const {
            return value_.get();
        }

        inline const auto &terminator() const {
            return terminator_.get();
        }
    };

    static_assert(nb::stream::is_finite_stream_writer_v<FixedResponse<1>>);

    template <size_t NCommand, size_t NResponse>
    class FixedQuery {
        FixedCommand<NCommand> command_;
        FixedResponse<NResponse> response_;

      public:
        using StreamReaderItem = uint8_t;
        using StreamWriterItem = uint8_t;

        FixedQuery() = delete;
        FixedQuery(const FixedQuery &) = default;
        FixedQuery(FixedQuery &&) = default;
        FixedQuery &operator=(const FixedQuery &) = default;
        FixedQuery &operator=(FixedQuery &&) = default;

        FixedQuery(nb::stream::FixedByteReader<NCommand> &&command_name) : command_{command_name} {}

        inline bool is_readable() const {
            return command_.is_readable();
        }

        inline size_t readable_count() const {
            return command_.readable_count();
        }

        inline etl::optional<StreamReaderItem> read() {
            return command_.read();
        }

        inline bool is_writable() const {
            return response_.is_writable();
        }

        inline size_t writable_count() const {
            return response_.writable_count();
        }

        inline bool write(uint8_t data) {
            return response_.write(data);
        }

        inline bool is_closed() const {
            return nb::stream::is_closed(command_, response_);
        }

        inline etl::optional<FixedResponse<NResponse>> response() const {
            if (is_closed()) {
                return response_;
            } else {
                return etl::nullopt;
            }
        }
    };

    static_assert(nb::stream::is_finite_stream_writer_v<FixedQuery<1, 1>>);
    static_assert(nb::stream::is_finite_stream_reader_v<FixedQuery<1, 1>>);

    template <typename T, size_t NCommand, size_t NResponse>
    class FixedExecutor {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        FixedQuery<NCommand, NResponse> query_;

      public:
        using StreamReaderItem = FixedResponse<NResponse>;

        FixedExecutor() = delete;
        FixedExecutor(const FixedExecutor &) = delete;
        FixedExecutor(FixedExecutor &&) = default;
        FixedExecutor &operator=(const FixedExecutor &) = delete;
        FixedExecutor &operator=(FixedExecutor &&) = default;

        FixedExecutor(
            nb::lock::Guard<nb::serial::Serial<T>> &&serial,
            FixedCommand<NCommand> &&command
        )
            : serial_{etl::move(serial)},
              query_{command} {}

        void execute() {
            nb::stream::pipe(query_, *serial_);
            nb::stream::pipe(*serial_, query_);
        }

        inline bool is_readable() const {
            return query_.is_closed();
        }

        inline size_t readable_count() const {
            return query_.is_closed() ? 1 : 0;
        }

        inline etl::optional<FixedResponse<NResponse>> read() {
            return query_.response();
        }

        inline bool is_closed() const {
            return query_.is_closed();
        }
    };

    static_assert(nb::stream::is_finite_stream_reader_v<FixedExecutor<mock::MockSerial, 1, 1>>);

    class TransmissionCommand {
        nb::stream::FixedByteReader<1> prefix_{'@'};
        nb::stream::FixedByteReader<2> command_name_{'D', 'T'};
        nb::stream::FixedByteReader<2> length_;
        nb::stream::HeapStreamReader<uint8_t> value_;
        nb::stream::FixedByteReader<2> terminator_{'\r', '\n'};

      public:
        using StreamReaderItem = uint8_t;

        TransmissionCommand() = delete;
        TransmissionCommand(const TransmissionCommand &) = delete;
        TransmissionCommand(TransmissionCommand &&) = default;
        TransmissionCommand &operator=(const TransmissionCommand &) = delete;
        TransmissionCommand &operator=(TransmissionCommand &&) = default;

        TransmissionCommand(nb::stream::HeapStreamReader<uint8_t> &&value)
            : length_{serde::hex::serialize(value.readable_count())},
              value_{etl::move(value)} {}

        bool is_readable() const {
            return nb::stream::is_readable(prefix_, command_name_, length_, value_, terminator_);
        }

        size_t readable_count() const {
            return nb::stream::readable_count(prefix_, command_name_, length_, value_, terminator_);
        }

        etl::optional<StreamReaderItem> read() {
            return nb::stream::read(prefix_, command_name_, length_, value_, terminator_);
        }

        bool is_closed() const {
            return terminator_.is_closed();
        }
    };

    static_assert(nb::stream::is_finite_stream_reader_v<TransmissionCommand>);

    template <typename T>
    class TransmissionQuery {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        TransmissionCommand command_;
        FixedResponse<2> response_;

      public:
        using StreamReaderItem = uint8_t;
        using StreamWriterItem = uint8_t;

        TransmissionQuery() = delete;
        TransmissionQuery(const TransmissionQuery &) = delete;
        TransmissionQuery(TransmissionQuery &&) = default;
        TransmissionQuery &operator=(const TransmissionQuery &) = delete;
        TransmissionQuery &operator=(TransmissionQuery &&) = default;

        TransmissionQuery(
            nb::lock::Guard<nb::serial::Serial<T>> &&serial,
            nb::stream::HeapStreamReader<uint8_t> &&value
        )
            : serial_{etl::move(serial)},
              command_{etl::move(value)} {}

        inline bool is_readable() const {
            return command_.is_readable();
        }

        inline size_t readable_count() const {
            return command_.readable_count();
        }

        inline etl::optional<StreamReaderItem> read() {
            return command_.read();
        }

        inline bool is_writable() const {
            return response_.is_writable();
        }

        inline size_t writable_count() const {
            return response_.writable_count();
        }

        inline bool write(uint8_t data) {
            return response_.write(data);
        }

        inline bool is_closed() const {
            return nb::stream::is_closed(command_, response_);
        }

        inline etl::optional<FixedResponse<2>> response() const {
            if (is_closed()) {
                return response_;
            } else {
                return etl::nullopt;
            }
        }
    };

    static_assert(nb::stream::is_finite_stream_writer_v<TransmissionQuery<mock::MockSerial>>);
    static_assert(nb::stream::is_finite_stream_reader_v<TransmissionQuery<mock::MockSerial>>);

    template <typename T>
    class TransmissionExecutor {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        TransmissionQuery<T> query_;

      public:
        using StreamReaderItem = FixedResponse<2>;

        TransmissionExecutor() = delete;
        TransmissionExecutor(const TransmissionExecutor &) = delete;
        TransmissionExecutor(TransmissionExecutor &&) = default;
        TransmissionExecutor &operator=(const TransmissionExecutor &) = delete;
        TransmissionExecutor &operator=(TransmissionExecutor &&) = default;

        TransmissionExecutor(
            nb::lock::Guard<nb::serial::Serial<T>> &&serial,
            nb::stream::HeapStreamReader<uint8_t> &&value
        )
            : serial_{etl::move(serial)},
              query_{etl::move(value)} {}

        void execute() {
            nb::stream::pipe(query_, *serial_);
            nb::stream::pipe(*serial_, query_);
        }

        inline bool is_readable() const {
            return query_.is_readable();
        }

        inline size_t readable_count() const {
            return query_.readable_count();
        }

        inline etl::optional<StreamReaderItem> read() {
            return query_.response();
        }

        inline bool is_closed() const {
            return query_.is_closed();
        }
    };

    static_assert(nb::stream::is_finite_stream_reader_v<TransmissionExecutor<mock::MockSerial>>);

    template <typename T>
    class ReceivingResponse {
        nb::stream::FixedByteWriter<1> prefix_;
        nb::stream::FixedByteWriter<1> command_name_;
        nb::stream::FixedByteWriter<1> equal_;
        nb::stream::FixedByteWriter<2> length_;
        etl::optional<nb::stream::HeapStreamWriter<uint8_t>> value_{etl::nullopt};
        nb::stream::FixedByteWriter<2> terminator_;

      public:
        using StreamWriterItem = uint8_t;

        ReceivingResponse() = default;
        ReceivingResponse(const ReceivingResponse &) = default;
        ReceivingResponse(ReceivingResponse &&) = default;
        ReceivingResponse &operator=(const ReceivingResponse &) = default;
        ReceivingResponse &operator=(ReceivingResponse &&) = default;

        bool is_writable() const {
            if (!value_.has_value()) {
                return nb::stream::is_writable(*value_, terminator_);
            }
            return nb::stream::is_writable(prefix_, command_name_, equal_, length_);
        }

        size_t writable_count() const {
            if (!value_.has_value()) {
                return nb::stream::writable_count(*value_, terminator_);
            }
            return nb::stream::writable_count(prefix_, command_name_, equal_, length_);
        }

        bool write(uint8_t data) {
            if (nb::stream::write(data, prefix_, command_name_, equal_, length_)) {
                return true;
            }
            if (!value_.has_value()) {
                auto length = serde::hex::deserialize<uint8_t>(length_.get());
                value_ = nb::stream::HeapStreamWriter<uint8_t>{*length};
            }
            return nb::stream::write(data, *value_, terminator_);
        }

        bool is_closed() const {
            return terminator_.is_closed();
        }
    };

    static_assert(nb::stream::is_finite_stream_writer_v<ReceivingResponse<mock::MockSerial>>);

    template <typename T>
    class ReceivingExecutor {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        ReceivingResponse<T> response_;

      public:
        using StreamReaderItem = ReceivingResponse<T>;

        ReceivingExecutor() = delete;
        ReceivingExecutor(const ReceivingExecutor &) = delete;
        ReceivingExecutor(ReceivingExecutor &&) = default;
        ReceivingExecutor &operator=(const ReceivingExecutor &) = delete;
        ReceivingExecutor &operator=(ReceivingExecutor &&) = default;

        ReceivingExecutor(nb::lock::Guard<nb::serial::Serial<T>> &&serial)
            : serial_{etl::move(serial)},
              response_{} {}

        void execute() {
            nb::stream::pipe(*serial_, response_);
        }

        inline bool is_readable() const {
            return response_.is_closed();
        }

        inline size_t readable_count() const {
            return response_.is_closed() ? 1 : 0;
        }

        inline etl::optional<ReceivingResponse<T>> read() {
            if (response_.is_closed()) {
                return response_;
            } else {
                return etl::nullopt;
            }
        }

        bool is_closed() const {
            return response_.is_closed();
        }
    };

    static_assert(nb::stream::is_finite_stream_reader_v<ReceivingExecutor<mock::MockSerial>>);

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

        etl::optional<FixedExecutor<T, 0, 9>> get_serial_number() {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            };
            auto command = FixedCommand<0>{{'S', 'N'}, {}};
            return {etl::move(guard), command};
        }

        etl::optional<FixedExecutor<T, 2, 2>> set_equipment_id(const UHFId &id) {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            FixedCommand<2> command{{'E', 'I'}, {id.value()}};
            return FixedExecutor{etl::move(guard), command};
        }

        etl::optional<FixedExecutor<T, 2, 2>> set_destination_id(const UHFId &id) {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            FixedCommand<2> command{{'D', 'I'}, {id.value()}};
            return FixedExecutor{etl::move(guard), command};
        }

        etl::optional<FixedExecutor<T, 0, 2>> carrier_sense() {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            FixedCommand<2> command{{'C', 'S'}, {}};
            return FixedExecutor{etl::move(guard), command};
        }

        etl::optional<TransmissionExecutor<T>>
        send_packet(nb::stream::HeapStreamReader<uint8_t> &&stream) {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            TransmissionCommand command{etl::move(stream)};
            return TransmissionExecutor{etl::move(guard), etl::move(command)};
        }

        etl::optional<ReceivingExecutor<T>> receive_packet() {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            return ReceivingExecutor{etl::move(guard)};
        }
    };
} // namespace media::uhf::modem
