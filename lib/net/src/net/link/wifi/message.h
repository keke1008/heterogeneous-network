#pragma once

#include "./message/receve_data.h"
#include "./message/unknown.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <nb/stream.h>
#include <stdint.h>
#include <util/span.h>

namespace net::link::wifi {
    enum class MessageType : uint8_t {
        Unknown,
        DataReceived,
    };

    class MessageDetector {
        nb::stream::MinLengthSingleLineWritableBuffer<5> buffer_;
        etl::optional<nb::stream::DiscardSingleLineWritableBuffer> discarder_;

      public:
        nb::Poll<MessageType> execute(nb::stream::ReadableStream &stream) {
            if (discarder_.has_value()) {
                POLL_UNWRAP_OR_RETURN(discarder_.value().write_all_from(stream));
                discarder_.reset();
                return nb::ready(MessageType::Unknown);
            }

            POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream));
            if (util::as_str(buffer_.written_bytes()) == "+IPD,") {
                return nb::ready(MessageType::DataReceived);
            } else {
                discarder_.emplace();
                return nb::pending;
            }
        }
    };

    class MessageHandler {
        etl::variant<MessageDetector, DiscardUnknownMessage, ReceiveDataMessageHandler> task_{};
        etl::optional<Frame> received_frame_;
        bool discard_frame_;

      public:
        explicit MessageHandler(bool discard_frame) : discard_frame_{discard_frame} {}

        inline nb::Poll<Frame> receive_frame() {
            if (received_frame_.has_value()) {
                auto frame = etl::move(received_frame_);
                received_frame_.reset();
                return etl::move(frame.value());
            } else {
                return nb::pending;
            }
        }

        template <net::frame::IFrameService FrameService>
        nb::Poll<etl::optional<Frame>>
        execute(FrameService &service, nb::stream::ReadableStream &stream) {
            if (etl::holds_alternative<MessageDetector>(task_)) {
                auto &task = etl::get<MessageDetector>(task_);
                auto type = POLL_UNWRAP_OR_RETURN(task.execute(stream));
                switch (type) {
                case MessageType::Unknown: {
                    task_.emplace<DiscardUnknownMessage>();
                }
                case MessageType::DataReceived: {
                    bool discard = received_frame_.has_value();
                    auto task = ReceiveDataMessageHandler{discard};
                    task_.emplace<ReceiveDataMessageHandler>(etl::move(task), discard_frame_);
                }
                }
            }

            if (etl::holds_alternative<DiscardUnknownMessage>(task_)) {
                auto &task = etl::get<DiscardUnknownMessage>(task_);
                POLL_UNWRAP_OR_RETURN(task.execute(stream));
                return nb::ready(etl::optional<Frame>{});
            }

            if (etl::holds_alternative<ReceiveDataMessageHandler>(task_)) {
                auto &task = etl::get<ReceiveDataMessageHandler>(task_);
                return task.execute(service, stream);
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
