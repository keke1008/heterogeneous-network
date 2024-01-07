#pragma once

#include "../../broker.h"
#include "../common.h"
#include "../response.h"
#include <nb/serde.h>
#include <net/frame.h>
#include <stdint.h>

namespace net::link::uhf {
    struct DRParameter {
        uint8_t length;
        frame::ProtocolNumber protocol;

        inline uint8_t payload_length() const {
            return length - frame::PROTOCOL_SIZE;
        }
    };

    class AsyncDRParameterDeserializer {
        nb::de::Hex<uint8_t> length_;
        frame::AsyncProtocolNumberDeserializer protocol_;

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

    class GetFrameWriter {
        uint8_t length_;

      public:
        explicit GetFrameWriter(const DRParameter &param) : length_{param.payload_length()} {}

        inline nb::Poll<frame::FrameBufferWriter> execute(frame::FrameService &service) {
            return service.request_frame_writer(length_);
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
            GetFrameWriter,
            frame::AsyncFrameBufferWriterDeserializer,
            AsyncDRTrailerDeserializer>
            state_{};

        etl::optional<frame::ProtocolNumber> protocol_;
        etl::optional<frame::FrameBufferReader> reader_;

      public:
        ReceiveDataTask(const ReceiveDataTask &) = delete;
        ReceiveDataTask(ReceiveDataTask &&) = default;
        ReceiveDataTask &operator=(const ReceiveDataTask &) = delete;
        ReceiveDataTask &operator=(ReceiveDataTask &&) = default;

        explicit ReceiveDataTask(nb::LockGuard<etl::reference_wrapper<R>> &&rw)
            : rw_{etl::move(rw)} {}

        nb::Poll<void> execute(frame::FrameService &fs, FrameBroker &broker, util::Time &time) {
            auto &rw = rw_->get();

            if (etl::holds_alternative<AsyncDRParameterDeserializer>(state_)) {
                auto &state = etl::get<AsyncDRParameterDeserializer>(state_);
                if (POLL_UNWRAP_OR_RETURN(state.deserialize(rw)) != nb::de::DeserializeResult::Ok) {
                    return nb::ready();
                }

                const auto &param = state.result();
                protocol_ = param.protocol;
                state_.emplace<GetFrameWriter>(param);
            }

            if (etl::holds_alternative<GetFrameWriter>(state_)) {
                auto &state = etl::get<GetFrameWriter>(state_);
                auto &&writer = POLL_MOVE_UNWRAP_OR_RETURN(state.execute(fs));
                state_.emplace<frame::AsyncFrameBufferWriterDeserializer>(etl::move(writer));
            }

            if (etl::holds_alternative<frame::AsyncFrameBufferWriterDeserializer>(state_)) {
                auto &state = etl::get<frame::AsyncFrameBufferWriterDeserializer>(state_);
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
                    LinkFrame{
                        .protocol_number = *protocol_,
                        .remote = LinkAddress(source_id),
                        .reader = etl::move(reader_.value()),
                    },
                    time
                );

                return nb::ready();
            }

            return nb::pending;
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<R> &&_) {
            return UhfHandleResponseResult::Invalid;
        }
    };
} // namespace net::link::uhf
