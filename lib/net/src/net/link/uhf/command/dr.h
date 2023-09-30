#pragma once

#include "../../link.h"
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
        uint8_t protocol;

        static DRHeader parse(etl::span<const uint8_t> span) {
            nb::buf::BufferSplitter splitter{span};
            splitter.split_nbytes<4>(); // "@DR="
            return DRHeader{
                .length = serde::hex::deserialize<uint8_t>(splitter.split_nbytes<2>()).value(),
                .protocol = splitter.split_1byte(),
            };
        }
    };

    struct DRTrailer {
        static constexpr uint8_t SIZE = /*route prefix*/ 2 + /* route  */ 2 + /* suffix  */ 2;

        ModemId source;

        static DRTrailer parse(etl::span<const uint8_t> span) {
            nb::buf::BufferSplitter splitter{span};

            splitter.split_nbytes<2>(); // "/R"
            ModemId source = splitter.parse<ModemIdParser>();
            splitter.split_nbytes<2>(); // "\r\n"

            return DRTrailer{.source = source};
        }
    };

    class DRExecutor {
        enum class State : uint8_t {
            Header,
            Body,
            Trailer,
            Complete,
        } state_{State::Header};

        nb::stream::FixedWritableBuffer<DRHeader::SIZE> header_;
        etl::optional<net::frame::FrameReception<Address>> body_;
        nb::stream::FixedWritableBuffer<DRTrailer::SIZE> trailer_;

      public:
        DRExecutor() = default;
        DRExecutor(const DRExecutor &) = delete;
        DRExecutor(DRExecutor &&) = default;
        DRExecutor &operator=(const DRExecutor &) = delete;
        DRExecutor &operator=(DRExecutor &&) = default;

        template <net::frame::IFrameService<Address> FrameService>
        nb::Poll<void> poll(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (state_ == State::Header) {
                POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream););
                auto header = DRHeader::parse(header_.written_bytes());
                body_ = POLL_MOVE_UNWRAP_OR_RETURN(
                    service.notify_reception(header.protocol, header.length - frame::PROTOCOL_SIZE)
                );
                state_ = State::Body;
            }

            if (state_ == State::Body) {
                POLL_UNWRAP_OR_RETURN(body_->writer.write_all_from(stream));
                state_ = State::Trailer;
            }

            if (state_ == State::Trailer) {
                POLL_UNWRAP_OR_RETURN(trailer_.write_all_from(stream));
                auto trailer = DRTrailer::parse(trailer_.written_bytes());
                body_->source.set_value(Address(trailer.source));
                state_ = State::Complete;
            }

            return nb::ready();
        }
    };
} // namespace net::link::uhf
