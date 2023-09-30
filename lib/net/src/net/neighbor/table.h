#pragma once

#include <net/link.h>
#include <stdint.h>

namespace net::neighbor {
    constexpr uint8_t MAX_NEIGHBOR_COUNT = 10;

    struct Neighbor {
        link::Address address;
    };

    class NeighborTable {
        etl::vector<Neighbor, MAX_NEIGHBOR_COUNT> neighbors_;

      public:
        NeighborTable() = default;
        NeighborTable(const NeighborTable &) = delete;
        NeighborTable(NeighborTable &&) = default;
        NeighborTable &operator=(const NeighborTable &) = delete;
        NeighborTable &operator=(NeighborTable &&) = default;

        inline bool full() const {
            return neighbors_.full();
        }

        inline bool add_neighbor(const Neighbor &neighbor) {
            if (neighbors_.full()) {
                return false;
            }

            bool exists = etl::any_of(neighbors_.begin(), neighbors_.end(), [&](const Neighbor &n) {
                return n.address == neighbor.address;
            });
            if (!exists) {
                neighbors_.push_back(neighbor);
            }
            return true;
        }

        inline void remove_neighbor(const link::Address &address) {
            etl::remove_if(neighbors_.begin(), neighbors_.end(), [&](const Neighbor &n) {
                return n.address == address;
            });
        }
    };
} // namespace net::neighbor
