#pragma once

#include <nb/serde.h>
#include <net/notification.h>
#include <stdint.h>

namespace net::observer {
    enum class FrameType : uint8_t {
        NodeSubscription,
        NodeNotification,
    };

    static constexpr bool is_valid_frame_type(uint8_t frame_type) {
        return frame_type == static_cast<uint8_t>(FrameType::NodeSubscription) ||
            frame_type == static_cast<uint8_t>(FrameType::NodeNotification);
    }

    class AsyncFrameTypeDeserializer {
        nb::de::Bin<uint8_t> frame_type_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &buffer) {
            POLL_UNWRAP_OR_RETURN(frame_type_.deserialize(buffer));
            return is_valid_frame_type(frame_type_.result()) ? nb::DeserializeResult::Ok
                                                             : nb::DeserializeResult::Invalid;
        }

        inline FrameType result() const {
            ASSERT(is_valid_frame_type(frame_type_.result()));
            return static_cast<FrameType>(frame_type_.result());
        }
    };

    class AsyncNodeNotificationSerializer {
        nb::ser::Bin<uint8_t> frame_type_{static_cast<uint8_t>(FrameType::NodeNotification)};
        notification::AsyncNodeNotificationSerializer notification_;

      public:
        explicit AsyncNodeNotificationSerializer(const notification::Notification &notification)
            : notification_{notification} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(frame_type_.serialize(buffer));
            return notification_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return frame_type_.serialized_length() + notification_.serialized_length();
        }
    };
} // namespace net::observer
