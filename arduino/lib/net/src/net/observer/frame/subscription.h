#pragma once

#include <nb/serde.h>

namespace net::observer {
    struct NodeSubscriptionFrame {};

    class AsyncNodeSubscriptionFrameDeserializer {

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &buffer) {
            return nb::DeserializeResult::Ok;
        }

        inline NodeSubscriptionFrame result() const {
            return NodeSubscriptionFrame{};
        }
    };
} // namespace net::observer
