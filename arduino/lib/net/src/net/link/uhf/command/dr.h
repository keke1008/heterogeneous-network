#pragma once

#include "../../media.h"
#include "../common.h"
#include <etl/functional.h>
#include <nb/buf.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <net/frame/service.h>
#include <serde/hex.h>

namespace net::link::uhf {
    struct DRHeader {
        static constexpr uint8_t SIZE = /* prefix  */ 4 + /* length  */ 2 + /* protocol  */ 1;

        uint8_t length;
        frame::ProtocolNumber protocol;
    };

    struct DRHeaderParser {
        inline DRHeader parse(nb::buf::BufferSplitter &splitter) {
            splitter.split_nbytes<4>(); // "@DR="
            return DRHeader{
                .length = splitter.parse<nb::buf::HexParser<uint8_t>>(),
                .protocol = splitter.parse<frame::ProtocolNumberParser>(),
            };
        }
    };

    struct DRTrailer {
        static constexpr uint8_t SIZE = /*route prefix*/ 2 + /* route  */ 2 + /* suffix  */ 2;
        ModemId source;
    };

    struct DRTrailerParser {
        DRTrailer parse(nb::buf::BufferSplitter &splitter) {
            splitter.split_nbytes<2>(); // "/R"
            ModemId source = splitter.parse<ModemIdParser>();
            splitter.split_nbytes<2>(); // "\r\n"
            return DRTrailer{.source = source};
        }
    };

    class ParseDRHeader {
        nb::stream::FixedWritableBuffer<DRHeader::SIZE> buffer_;

      public:
        nb::Poll<DRHeader> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream));
            return nb::buf::parse<DRHeaderParser>(buffer_.written_bytes());
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

        nb::Poll<frame::FrameBufferReader> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(writer_.write_all_from(stream));
            return writer_.make_initial_reader();
        }
    };

    class ParseDRTrailer {
        nb::stream::FixedWritableBuffer<DRTrailer::SIZE> buffer_;

      public:
        nb::Poll<DRTrailer> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream));
            return nb::buf::parse<DRTrailerParser>(buffer_.written_bytes());
        }
    };

    class DRExecutor {
        etl::variant<ParseDRHeader, CreateFrameWriter, ReadFrameBody, ParseDRTrailer> state_{};
        etl::optional<DRHeader> header_;
        etl::optional<frame::FrameBufferReader> reader_;

      public:
        DRExecutor() = default;
        DRExecutor(const DRExecutor &) = delete;
        DRExecutor(DRExecutor &&) = default;
        DRExecutor &operator=(const DRExecutor &) = delete;
        DRExecutor &operator=(DRExecutor &&) = default;

        nb::Poll<UhfFrame>
        poll(frame::FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (etl::holds_alternative<ParseDRHeader>(state_)) {
                auto &state = etl::get<ParseDRHeader>(state_);
                header_ = POLL_UNWRAP_OR_RETURN(state.execute(stream));
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
                reader_ = POLL_MOVE_UNWRAP_OR_RETURN(state.execute(stream));
                state_ = ParseDRTrailer{};
            }

            if (!etl::holds_alternative<ParseDRTrailer>(state_)) {
                return nb::pending;
            }

            auto &state = etl::get<ParseDRTrailer>(state_);
            auto trailer = POLL_UNWRAP_OR_RETURN(state.execute(stream));
            return UhfFrame{
                .protocol_number = header_->protocol,
                .remote = trailer.source,
                .reader = etl::move(reader_.value()),
            };
        }
    };
} // namespace net::link::uhf
