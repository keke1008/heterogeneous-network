#pragma once

#include "../../media.h"
#include "../common.h"
#include <etl/functional.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/serde.h>
#include <net/frame/service.h>

namespace net::link::uhf {
    struct DRHeader {
        static constexpr uint8_t SIZE = /* prefix  */ 4 + /* length  */ 2 + /* protocol  */ 1;

        uint8_t length;
        frame::ProtocolNumber protocol;
    };

    class AsyncDRHeaderDeserializer {
        nb::de::SkipNBytes prefix_{4};
        nb::de::Hex<uint8_t> length_;
        nb::de::Bin<uint8_t> protocol_;

      public:
        inline DRHeader result() {
            return DRHeader{
                .length = length_.result(),
                .protocol = static_cast<frame::ProtocolNumber>(protocol_.result()),
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(prefix_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(length_.deserialize(reader));
            return protocol_.deserialize(reader);
        }
    };

    struct DRTrailer {
        static constexpr uint8_t SIZE = /*route prefix*/ 2 + /* route  */ 2 + /* suffix  */ 2;
        ModemId source;
    };

    class AsyncDRTrailerDeserializer {
        nb::de::SkipNBytes route_prefix_{2};
        nb::de::Hex<uint8_t> source_;
        nb::de::SkipNBytes suffix_{2};

      public:
        inline DRTrailer result() {
            return DRTrailer{
                .source = static_cast<ModemId>(source_.result()),
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(route_prefix_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(reader));
            return suffix_.deserialize(reader);
        }
    };

    class CreateFrameWriter {
        uint8_t length_;

      public:
        explicit CreateFrameWriter(uint8_t length) : length_{length} {}

        inline nb::Poll<frame::FrameBufferWriter> execute(frame::FrameService &service) {
            return service.request_frame_writer(length_);
        }
    };

    class ReadFrameBody {
        frame::FrameBufferWriter writer_;

      public:
        explicit ReadFrameBody(frame::FrameBufferWriter writer) : writer_{etl::move(writer)} {}

        template <nb::AsyncReadable R>
        nb::Poll<frame::FrameBufferReader> execute(R &r) {
            while (!writer_.is_all_written()) {
                POLL_UNWRAP_OR_RETURN(r.poll_readable(1));
                writer_.write(r.read_unchecked());
            }
            return writer_.create_reader();
        }
    };

    class DRExecutor {
        etl::variant<
            AsyncDRHeaderDeserializer,
            CreateFrameWriter,
            ReadFrameBody,
            AsyncDRTrailerDeserializer>
            state_{};
        etl::optional<DRHeader> header_;
        etl::optional<frame::FrameBufferReader> reader_;

      public:
        DRExecutor() = default;
        DRExecutor(const DRExecutor &) = delete;
        DRExecutor(DRExecutor &&) = default;
        DRExecutor &operator=(const DRExecutor &) = delete;
        DRExecutor &operator=(DRExecutor &&) = default;

        template <nb::AsyncReadableWritable RW>
        nb::Poll<UhfFrame> poll(frame::FrameService &service, RW &rw) {
            if (etl::holds_alternative<AsyncDRHeaderDeserializer>(state_)) {
                auto &state = etl::get<AsyncDRHeaderDeserializer>(state_);
                POLL_UNWRAP_OR_RETURN(state.deserialize(rw));
                header_ = state.result();
                uint8_t length = header_->length - frame::PROTOCOL_SIZE;
                state_ = CreateFrameWriter{length};
            }

            if (etl::holds_alternative<CreateFrameWriter>(state_)) {
                auto &state = etl::get<CreateFrameWriter>(state_);
                auto writer = POLL_MOVE_UNWRAP_OR_RETURN(state.execute(service));
                state_ = ReadFrameBody{etl::move(writer)};
            }

            if (etl::holds_alternative<ReadFrameBody>(state_)) {
                auto &state = etl::get<ReadFrameBody>(state_);
                reader_ = POLL_MOVE_UNWRAP_OR_RETURN(state.execute(rw));
                state_.emplace<AsyncDRTrailerDeserializer>();
            }

            if (etl::holds_alternative<AsyncDRTrailerDeserializer>(state_)) {
                auto &state = etl::get<AsyncDRTrailerDeserializer>(state_);
                POLL_UNWRAP_OR_RETURN(state.deserialize(rw));
                auto &&trailer = state.result();
                return UhfFrame{
                    .protocol_number = header_->protocol,
                    .remote = trailer.source,
                    .reader = etl::move(reader_.value()),
                };
            }

            return nb::pending;
        }
    };
} // namespace net::link::uhf
