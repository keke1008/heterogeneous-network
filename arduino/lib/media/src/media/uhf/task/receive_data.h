#pragma once

#include "../common.h"
#include <nb/lock.h>
#include <nb/serde.h>
#include <net/frame.h>
#include <net/link.h>
#include <stdint.h>

namespace media::uhf {
    struct DRParameter {
        uint8_t length;
        net::frame::ProtocolNumber protocol;

        inline uint8_t payload_length() const {
            return length - net::frame::PROTOCOL_SIZE;
        }
    };

    class AsyncDRParameterDeserializer {
        nb::de::Hex<uint8_t> length_;
        net::frame::AsyncProtocolNumberDeserializer protocol_;

      public:
        template <nb::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(length_.deserialize(readable));
            SERDE_DESERIALIZE_OR_RETURN(protocol_.deserialize(readable));
            return nb::de::DeserializeResult::Ok;
        }

        inline DRParameter result() const {
            return DRParameter{
                .length = length_.result(),
                .protocol = protocol_.result(),
            };
        }
    };

    class AsyncDRTrailerDeserializer {
        nb::de::SkipNBytes route_prefix_{2};
        AsyncModemIdDeserializer source_;
        nb::de::SkipNBytes suffix_{2};

      public:
        template <nb::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(route_prefix_.deserialize(readable));
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(readable));
            return suffix_.deserialize(readable);
        }

        inline ModemId result() const {
            return source_.result();
        }
    };

    template <nb::AsyncReadable R>
    class ReceiveDataTask {
        nb::LockGuard<etl::reference_wrapper<R>> rw_;
        etl::variant<
            AsyncDRParameterDeserializer,
            net::frame::AsyncFrameBufferWriterDeserializer,
            AsyncDRTrailerDeserializer>
            state_{};

        etl::optional<net::frame::ProtocolNumber> protocol_;
        etl::optional<net::frame::FrameBufferReader> reader_;

      public:
        ReceiveDataTask(const ReceiveDataTask &) = delete;
        ReceiveDataTask(ReceiveDataTask &&) = default;
        ReceiveDataTask &operator=(const ReceiveDataTask &) = delete;
        ReceiveDataTask &operator=(ReceiveDataTask &&) = default;

        explicit ReceiveDataTask(nb::LockGuard<etl::reference_wrapper<R>> &&rw)
            : rw_{etl::move(rw)} {}

        nb::Poll<void>
        execute(net::frame::FrameService &fs, net::link::FrameBroker &broker, util::Time &time) {
            auto &rw = rw_->get();

            if (etl::holds_alternative<AsyncDRParameterDeserializer>(state_)) {
                auto &state = etl::get<AsyncDRParameterDeserializer>(state_);
                if (POLL_UNWRAP_OR_RETURN(state.deserialize(rw)) != nb::de::DeserializeResult::Ok) {
                    return nb::ready();
                }

                const auto &param = state.result();
                protocol_ = param.protocol;

                auto &&poll_writer = fs.request_frame_writer(param.payload_length());
                if (poll_writer.is_pending()) {
                    LOG_INFO(FLASH_STRING("Ethernet: no writer, discard frame"));
                    return nb::ready();
                }

                auto &&writer = poll_writer.unwrap();
                reader_ = writer.create_reader();
                state_.emplace<net::frame::AsyncFrameBufferWriterDeserializer>(etl::move(writer));
            }

            if (etl::holds_alternative<net::frame::AsyncFrameBufferWriterDeserializer>(state_)) {
                auto &state = etl::get<net::frame::AsyncFrameBufferWriterDeserializer>(state_);
                if (POLL_UNWRAP_OR_RETURN(state.deserialize(rw)) != nb::de::DeserializeResult::Ok) {
                    return nb::ready();
                }
                reader_ = etl::move(state).result().create_reader();
                state_.emplace<AsyncDRTrailerDeserializer>();
            }

            if (etl::holds_alternative<AsyncDRTrailerDeserializer>(state_)) {
                auto &state = etl::get<AsyncDRTrailerDeserializer>(state_);
                if (POLL_UNWRAP_OR_RETURN(state.deserialize(rw)) != nb::de::DeserializeResult::Ok) {
                    return nb::ready();
                }

                auto source_id = state.result();
                broker.poll_dispatch_received_frame(
                    *protocol_, net::link::Address(source_id), etl::move(*reader_), time
                );

                return nb::ready();
            }

            return nb::pending;
        }
    };
} // namespace media::uhf