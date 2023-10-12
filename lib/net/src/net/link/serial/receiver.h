#pragma once

#include "../media.h"
#include "./layout.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class ReceiveHeader {
        nb::stream::RepetitionCountWritableBuffer preamble_{PREAMBLE, PREAMBLE_LENGTH};
        nb::stream::FixedWritableBuffer<HEADER_LENGTH> header_;

      public:
        nb::Poll<FrameHeader> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(preamble_.write_all_from(stream));
            POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
            return nb::buf::parse<FrameHeaderParser>(header_.written_bytes());
        }
    };

    class RequestFrameWriter {
        uint8_t length_;

      public:
        inline explicit RequestFrameWriter(uint8_t length) : length_{length} {}

        template <net::frame::IFrameService FrameService>
        inline nb::Poll<frame::FrameBufferWriter> execute(FrameService &service) {
            return service.request_frame_writer(length_);
        }
    };

    class ReceiveData {
        frame::FrameBufferWriter frame_writer_;

      public:
        inline explicit ReceiveData(frame::FrameBufferWriter &&frame_writer)
            : frame_writer_{etl::move(frame_writer)} {}

        template <net::frame::IFrameService FrameService>
        inline nb::Poll<void>
        execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            return frame_writer_.write_all_from(stream);
        }
    };

    class DiscardData {
        nb::stream::DiscardingCountWritableBuffer discarding_;

      public:
        inline explicit DiscardData(uint8_t length) : discarding_{length} {}

        inline nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            return discarding_.write_all_from(stream);
        }
    };

    class FrameReceiver {
        etl::variant<ReceiveHeader, RequestFrameWriter, ReceiveData, DiscardData> state_{};
        etl::optional<FrameHeader> header_;
        etl::optional<Frame> received_frame_;
        SerialAddress address_;

      public:
        explicit FrameReceiver(const SerialAddress &address) : address_{address} {}

        nb::Poll<Frame> receive_frame() {
            if (received_frame_.has_value()) {
                auto frame = etl::move(received_frame_.value());
                received_frame_ = etl::nullopt;
                return frame;
            } else {
                return nb::pending;
            }
        }

      private:
        // `POLL_UNWRAP_OR_RETURN` を利用するためのラッパー
        template <net::frame::IFrameService FrameService>
        nb::Poll<void>
        execute_inner(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (etl::holds_alternative<ReceiveHeader>(state_)) {
                auto &state = etl::get<ReceiveHeader>(state_);
                header_ = POLL_UNWRAP_OR_RETURN(state.execute(stream));

                if (received_frame_.has_value() || header_->destination != address_) {
                    state_ = DiscardData{header_->length};
                } else {
                    state_ = RequestFrameWriter{header_->length};
                }
            }

            if (etl::holds_alternative<RequestFrameWriter>(state_)) {
                auto &state = etl::get<RequestFrameWriter>(state_);
                auto writer = POLL_MOVE_UNWRAP_OR_RETURN(state.execute(service));
                auto reader = writer.make_initial_reader();
                state_ = ReceiveData{etl::move(writer)};
                received_frame_.emplace(
                    header_->protocol_number, Address{header_->source}, header_->length,
                    etl::move(reader)
                );
                header_ = etl::nullopt;
            }

            if (etl::holds_alternative<ReceiveData>(state_)) {
                auto &state = etl::get<ReceiveData>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute(service, stream));
                state_ = ReceiveHeader{};
            }

            if (etl::holds_alternative<DiscardData>(state_)) {
                auto &state = etl::get<DiscardData>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute(stream));
                state_ = ReceiveHeader{};
            }

            return nb::pending;
        }

      public:
        template <net::frame::IFrameService FrameService>
        inline void execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            execute_inner(service, stream);
        }
    };
} // namespace net::link::serial
