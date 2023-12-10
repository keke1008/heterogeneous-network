#pragma once

#include "./message/receive_data.h"
#include "./message/unknown.h"
#include "./message/wifi.h"
#include <etl/optional.h>
#include <etl/span.h>
#include <stdint.h>
#include <util/span.h>

namespace net::link::wifi {
    enum class MessageType : uint8_t {
        Unknown,
        Wifi,
        DataReceived,
    };

    class MessageDetector {
        nb::de::AsyncMinLengthSingleLineBytesDeserializer<5> buffer_;
        etl::optional<nb::de::AsyncDiscardingSingleLineDeserializer> discarder_;

      public:
        template <nb::AsyncReadable R>
        nb::Poll<MessageType> execute(R &r) {
            if (discarder_.has_value()) {
                POLL_UNWRAP_OR_RETURN(discarder_->deserialize(r));
                discarder_.reset();
                return nb::ready(MessageType::Unknown);
            }

            auto result = POLL_UNWRAP_OR_RETURN(buffer_.deserialize(r));
            if (result != nb::DeserializeResult::Ok) {
                discarder_.emplace();
                return nb::pending;
            }

            auto buffer = util::as_str(buffer_.result());
            if (buffer == "+IPD,") {
                return nb::ready(MessageType::DataReceived);
            }
            if (buffer == "WIFI ") {
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

      public:
        MessageHandler() = default;

        template <nb::AsyncReadableWritable RW>
        nb::Poll<etl::optional<WifiEvent>> execute(frame::FrameService &service, RW &rw) {
            if (etl::holds_alternative<MessageDetector>(task_)) {
                auto &task = etl::get<MessageDetector>(task_);
                auto type = POLL_UNWRAP_OR_RETURN(task.execute(rw));
                switch (type) {
                case MessageType::DataReceived: {
                    task_.emplace<ReceiveDataMessageHandler>();
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

            using Result = nb::Poll<etl::optional<WifiEvent>>;
            return etl::visit<Result>(
                util::Visitor{
                    [&](MessageDetector &task) -> Result { return nb::pending; },
                    [&](DiscardUnknownMessage &task) -> Result {
                        POLL_UNWRAP_OR_RETURN(task.execute(rw));
                        return etl::optional<WifiEvent>{};
                    },
                    [&](ReceiveDataMessageHandler &task) { return task.execute(service, rw); },
                    [&](WifiMessageHandler &task) { return task.execute(rw); },
                },
                task_
            );
        }
    };
} // namespace net::link::wifi
