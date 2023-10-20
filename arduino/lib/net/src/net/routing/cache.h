#pragma once

#include "./node.h"
#include <etl/circular_buffer.h>

namespace net::routing {
    struct GatewayCacheEntry {
        NodeId destination_id;
        link::Address gateway_address;
    };

    class GatewayCache {
        static constexpr uint8_t MAX_ENTRIES = 4;
        etl::circular_buffer<GatewayCacheEntry, MAX_ENTRIES> entries_;

      public:
        GatewayCache() = default;

        inline void add(const NodeId &destination_id, const link::Address &gateway_address) {
            entries_.push(GatewayCacheEntry{destination_id, gateway_address});
        }

        inline etl::optional<link::Address> get(const NodeId &destination_id) const {
            auto entry = etl::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
                return entry.destination_id == destination_id;
            });
            if (entry != entries_.end()) {
                return entry->gateway_address;
            } else {
                return etl::nullopt;
            }
        }

        inline void remove(const NodeId &destination_id) {
            for (uint8_t i = 0; i < entries_.size(); ++i) {
                if (entries_[i].destination_id == destination_id) {
                    entries_[i] = entries_.front();
                    entries_.pop();
                }
            }
        }
    };
}; // namespace net::routing
