#pragma once

#include <nb/serde.h>
#include <stdint.h>

namespace net::observer {
    enum class FrameType : uint8_t {
        NodeSubscription = 1,
        NodeNotification = 2,
        NodeSync = 3,
    };

    static constexpr bool is_valid_frame_type(uint8_t frame_type) {
        return frame_type == static_cast<uint8_t>(FrameType::NodeSubscription) ||
            frame_type == static_cast<uint8_t>(FrameType::NodeNotification);
    }

    using AsyncFrameTypeDeserializer = nb::de::Enum<FrameType, is_valid_frame_type>;
    using AsyncFrameTypeSerializer = nb::ser::Enum<FrameType>;
} // namespace net::observer
