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
    class ModemId {
        etl::array<uint8_t, 2> value_;

      public:
        ModemId() = delete;
        ModemId(const ModemId &) = default;
        ModemId(ModemId &&) = default;
        ModemId &operator=(const ModemId &) = default;
        ModemId &operator=(ModemId &&) = default;

        ModemId(const etl::array<uint8_t, 2> &value) : value_{value} {}

        template <typename... Ts>
        ModemId(Ts... args) : value_{args...} {}

        bool operator==(const ModemId &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const ModemId &other) const {
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
              value_{value} {}

        bool is_readable() const {
            return terminator_.is_readable();
        }

        size_t readable_count() const {
            return nb::stream::readable_count(prefix_, command_name_, value_, terminator_);
        }

        etl::optional<uint8_t> read() {
            return nb::stream::read(prefix_, command_name_, value_, terminator_);
        }

        bool is_closed() const {
            return terminator_.is_closed();
        }
    };

    static_assert(nb::stream::is_finite_stream_reader_v<FixedCommand<1>>);

    template <size_t N>
    class FixedResponse {
        nb::stream::FixedByteWriter<1> prefix_;
        nb::stream::FixedByteWriter<2> command_name_;
        nb::stream::FixedByteWriter<1> equal_;
        nb::stream::FixedByteWriter<N> data_;
        nb::stream::FixedByteWriter<2> terminator_;

      public:
        using StreamWriterItem = uint8_t;

        FixedResponse() = default;
        FixedResponse(const FixedResponse &) = default;
        FixedResponse(FixedResponse &&) = default;
        FixedResponse &operator=(const FixedResponse &) = default;
        FixedResponse &operator=(FixedResponse &&) = default;

        bool is_writable() const {
            return terminator_.is_writable();
        }

        size_t writable_count() const {
            return nb::stream::writable_count(prefix_, command_name_, equal_, data_, terminator_);
        }

        bool write(uint8_t data) {
            return nb::stream::write(data, prefix_, command_name_, equal_, data_, terminator_);
        }

        bool is_closed() const {
            return terminator_.is_closed();
        }

        inline const auto &prefix() const {
            return prefix_.get();
        }

        inline const auto &command_name() const {
            return command_name_.get();
        }

        inline const auto &equal() const {
            return equal_.get();
        }

        inline const auto &data() const {
            return data_.get();
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

        FixedQuery(FixedCommand<NCommand> &&command) : command_{command} {}

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
              query_{etl::move(command)} {}

        inline etl::optional<FixedResponse<NResponse>> execute() {
            nb::stream::pipe(query_, *serial_);
            nb::stream::pipe(*serial_, query_);
            return query_.response();
        }
    };

    class TransmissionCommand {
        nb::stream::FixedByteReader<1> prefix_{'@'};
        nb::stream::FixedByteReader<2> command_name_{'D', 'T'};
        nb::stream::FixedByteReader<2> length_;
        nb::stream::HeapStreamReader<uint8_t> data_;
        nb::stream::FixedByteReader<2> terminator_{'\r', '\n'};

      public:
        using StreamReaderItem = uint8_t;

        TransmissionCommand() = delete;
        TransmissionCommand(const TransmissionCommand &) = delete;
        TransmissionCommand(TransmissionCommand &&) = default;
        TransmissionCommand &operator=(const TransmissionCommand &) = delete;
        TransmissionCommand &operator=(TransmissionCommand &&) = default;

        TransmissionCommand(uint8_t length, nb::stream::HeapStreamReader<uint8_t> &&data_)
            : length_{serde::hex::serialize<uint8_t>(length)},
              data_{etl::move(data_)} {}

        bool is_readable() const {
            return terminator_.is_readable();
        }

        size_t readable_count() const {
            return nb::stream::readable_count(prefix_, command_name_, length_, data_, terminator_);
        }

        etl::optional<StreamReaderItem> read() {
            return nb::stream::read(prefix_, command_name_, length_, data_, terminator_);
        }

        bool is_closed() const {
            return terminator_.is_closed();
        }
    };

    static_assert(nb::stream::is_finite_stream_reader_v<TransmissionCommand>);

    class TransmissionQuery {
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

        TransmissionQuery(TransmissionCommand &&command) : command_{etl::move(command)} {}

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

    static_assert(nb::stream::is_finite_stream_writer_v<TransmissionQuery>);
    static_assert(nb::stream::is_finite_stream_reader_v<TransmissionQuery>);

    template <typename T>
    class TransmissionExecutor {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        TransmissionQuery query_;

      public:
        TransmissionExecutor() = delete;
        TransmissionExecutor(const TransmissionExecutor &) = delete;
        TransmissionExecutor(TransmissionExecutor &&) = default;
        TransmissionExecutor &operator=(const TransmissionExecutor &) = delete;
        TransmissionExecutor &operator=(TransmissionExecutor &&) = default;

        TransmissionExecutor(
            nb::lock::Guard<nb::serial::Serial<T>> &&serial,
            TransmissionCommand &&command
        )
            : serial_{etl::move(serial)},
              query_{etl::move(command)} {}

        inline etl::optional<FixedResponse<2>> execute() {
            nb::stream::pipe(query_, *serial_);
            nb::stream::pipe(*serial_, query_);
            return query_.response();
        }
    };

    class ReceivingResponse {
        nb::stream::FixedByteWriter<1> prefix_;
        nb::stream::FixedByteWriter<2> command_name_;
        nb::stream::FixedByteWriter<1> equal_;
        nb::stream::FixedByteWriter<2> length_;
        etl::optional<nb::stream::HeapStreamWriter<uint8_t>> data_{etl::nullopt};
        nb::stream::FixedByteWriter<2> terminator_;

      public:
        using StreamWriterItem = uint8_t;

        ReceivingResponse() = default;
        ReceivingResponse(const ReceivingResponse &) = delete;
        ReceivingResponse(ReceivingResponse &&) = default;
        ReceivingResponse &operator=(const ReceivingResponse &) = delete;
        ReceivingResponse &operator=(ReceivingResponse &&) = default;

        inline bool is_writable() const {
            return terminator_.is_writable();
        }

        size_t writable_count() const {
            if (data_.has_value()) {
                return nb::stream::writable_count(*data_, terminator_);
            }
            return nb::stream::writable_count(prefix_, command_name_, equal_, length_);
        }

        bool write(uint8_t data) {
            if (nb::stream::write(data, prefix_, command_name_, equal_, length_)) {
                return true;
            }
            if (!data_.has_value()) {
                auto length = serde::hex::deserialize<uint8_t>(length_.get());
                data_ = nb::stream::HeapStreamWriter<uint8_t>{*length};
            }
            return nb::stream::write(data, *data_, terminator_);
        }

        inline bool is_closed() const {
            return terminator_.is_closed();
        }

        inline const auto &prefix() const {
            return prefix_.get();
        }

        inline const auto &command_name() const {
            return command_name_.get();
        }

        inline const auto &equal() const {
            return equal_.get();
        }

        inline const auto &length() const {
            return length_.get();
        }

        inline const auto &data() const {
            return *data_;
        }

        inline const auto &terminator() const {
            return terminator_.get();
        }
    };

    static_assert(nb::stream::is_finite_stream_writer_v<ReceivingResponse>);

    template <typename T>
    class ReceivingExecutor {
        nb::lock::Guard<nb::serial::Serial<T>> serial_;
        ReceivingResponse response_;

      public:
        ReceivingExecutor() = delete;
        ReceivingExecutor(const ReceivingExecutor &) = delete;
        ReceivingExecutor(ReceivingExecutor &&) = default;
        ReceivingExecutor &operator=(const ReceivingExecutor &) = delete;
        ReceivingExecutor &operator=(ReceivingExecutor &&) = default;

        ReceivingExecutor(nb::lock::Guard<nb::serial::Serial<T>> &&serial)
            : serial_{etl::move(serial)},
              response_{} {}

        inline etl::optional<ReceivingResponse> execute() {
            nb::stream::pipe(*serial_, response_);
            if (response_.is_closed()) {
                return etl::optional(etl::move(response_));
            } else {
                return etl::nullopt;
            }
        }
    };

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
            return FixedExecutor<T, 0, 9>{etl::move(*guard), etl::move(command)};
        }

        etl::optional<FixedExecutor<T, 2, 2>> set_equipment_id(const ModemId &id) {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            FixedCommand<2> command{{'E', 'I'}, {id.value()}};
            return FixedExecutor<T, 2, 2>{etl::move(*guard), etl::move(command)};
        }

        etl::optional<FixedExecutor<T, 2, 2>> set_destination_id(const ModemId &id) {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            FixedCommand<2> command{{'D', 'I'}, {id.value()}};
            return FixedExecutor<T, 2, 2>{etl::move(*guard), etl::move(command)};
        }

        etl::optional<FixedExecutor<T, 0, 2>> carrier_sense() {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            FixedCommand<0> command{{'C', 'S'}, {}};
            return FixedExecutor<T, 0, 2>{etl::move(*guard), etl::move(command)};
        }

        etl::optional<TransmissionExecutor<T>>
        transmit_packet(uint8_t length, nb::stream::HeapStreamReader<uint8_t> &&stream) {
            auto guard = serial_.lock();
            if (!guard) {
                return etl::nullopt;
            }
            TransmissionCommand command{length, etl::move(stream)};
            return TransmissionExecutor<T>{etl::move(*guard), etl::move(command)};
        }

        etl::optional<ReceivingExecutor<T>> receive_packet() {
            auto guard = serial_.lock();
            if (guard.has_value() && (*guard)->is_readable()) {
                return ReceivingExecutor<T>{etl::move(*guard)};
            }
            return etl::nullopt;
        }
    };
} // namespace media::uhf::modem
