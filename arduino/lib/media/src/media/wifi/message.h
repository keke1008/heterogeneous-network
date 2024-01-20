#pragma once

#include "./message/receive_data.h"
#include "./message/unknown.h"
#include "./message/wifi.h"
#include <etl/optional.h>
#include <etl/span.h>
#include <stdint.h>
#include <util/span.h>

namespace media::wifi {
    enum class MessageType : uint8_t {
        Unknown,
        Wifi,
        TooShort,
        DataReceived,
    };

    class MessageDetector {
        nb::de::AsyncExceedLengthSingleLineBytesDeserializer<5> buffer_;

      public:
        template <nb::AsyncReadable R>
        nb::Poll<MessageType> execute(R &r) {
            auto result = POLL_UNWRAP_OR_RETURN(buffer_.deserialize(r));
            if (result != nb::DeserializeResult::Ok) {
                return MessageType::TooShort;
            }

            auto buffer = util::as_str(buffer_.result());
            if (buffer == "+IPD,") {
                return nb::ready(MessageType::DataReceived);
            }
            if (buffer == "WIFI ") {
                return nb::ready(MessageType::Wifi);
            }

            return nb::ready(MessageType::Unknown);
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
        nb::Poll<etl::optional<WifiEvent>> execute(net::frame::FrameService &service, RW &rw) {
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
                case MessageType::TooShort: {
                    return etl::optional<WifiEvent>{};
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
} // namespace media::wifi
