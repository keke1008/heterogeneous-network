#pragma once

#include "./message/receve_data.h"
#include "./message/unknown.h"
#include "./message/wifi.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <nb/stream.h>
#include <stdint.h>
#include <util/span.h>

namespace net::link::wifi {
    enum class MessageType : uint8_t {
        Unknown,
        Wifi,
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
            }
            if (util::as_str(buffer_.written_bytes()) == "WIFI ") {
                return nb::ready(MessageType::Wifi);
            }

            discarder_.emplace();
            return nb::pending;
        }
    };

    class MessageHandler {
        etl::variant<
            MessageDetector,
            DiscardUnknownMessage,
            ReceiveDataMessageHandler,
            WifiMessageHandler>
            task_{};
        bool discard_frame_;

      public:
        explicit MessageHandler(bool discard_frame) : discard_frame_{discard_frame} {}

        using Result = etl::variant<etl::monostate, WifiFrame, WifiEvent>;

        template <net::frame::IFrameService FrameService>
        nb::Poll<etl::variant<etl::monostate, WifiFrame, WifiEvent>>
        execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (etl::holds_alternative<MessageDetector>(task_)) {
                auto &task = etl::get<MessageDetector>(task_);
                auto type = POLL_UNWRAP_OR_RETURN(task.execute(stream));
                switch (type) {
                case MessageType::DataReceived: {
                    task_.emplace<ReceiveDataMessageHandler>(discard_frame_);
                    break;
                }
                case MessageType::Wifi: {
                    task_.emplace<WifiMessageHandler>();
                    break;
                }
                case MessageType::Unknown:
                default: {
                    task_.emplace<DiscardUnknownMessage>();
                }
                }
            }

            return etl::visit<nb::Poll<Result>>(
                util::Visitor{
                    [&](MessageDetector &task) -> nb::Poll<Result> { return nb::pending; },
                    [&](DiscardUnknownMessage &task) -> nb::Poll<Result> {
                        POLL_UNWRAP_OR_RETURN(task.execute(stream));
                        return Result{etl::monostate{}};
                    },
                    [&](ReceiveDataMessageHandler &task) -> nb::Poll<Result> {
                        auto opt_frame = POLL_MOVE_UNWRAP_OR_RETURN(task.execute(service, stream));
                        return opt_frame.has_value() ? Result{etl::move(opt_frame.value())}
                                                     : Result{etl::monostate{}};
                    },
                    [&](WifiMessageHandler &task) -> nb::Poll<Result> {
                        auto opt_event = POLL_MOVE_UNWRAP_OR_RETURN(task.execute(stream));
                        return opt_event.has_value() ? Result{etl::move(opt_event.value())}
                                                     : Result{etl::monostate{}};
                    },
                },
                task_
            );
        }
    };
} // namespace net::link::wifi
