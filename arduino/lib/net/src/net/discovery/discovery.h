#pragma once

#include "./cache.h"
#include "./constants.h"
#include "./task.h"
#include <nb/time.h>
#include <net/neighbor.h>
#include <net/node.h>
#include <tl/vec.h>

namespace net::discovery {
    struct FoundGateway {
        node::NodeId gateway_id;
        TotalCost cost;

        inline void replace_if_cheaper(const node::NodeId &gateway_id_, TotalCost cost_) {
            if (cost_ < cost) {
                gateway_id = gateway_id_;
                cost = cost_;
            }
        }
    };

    class DiscoveryEntry {
        node::Destination destination_;
        util::Instant start_;
        nb::Delay timeout_;
        etl::optional<FoundGateway> gateway_{etl::nullopt};

      public:
        DiscoveryEntry(const node::Destination &destination, util::Instant start)
            : destination_{destination},
              start_{start},
              timeout_{start, DISCOVERY_FIRST_RESPONSE_TIMEOUT} {}

        inline const node::Destination &destination() const {
            return destination_;
        }

        inline const etl::optional<FoundGateway> &get_gateway() const {
            return gateway_;
        }

        inline bool is_expired(util::Instant now) const {
            return timeout_.poll(now).is_ready();
        }

        inline void
        on_gateway_found(util::Time &time, const node::NodeId &gateway_id, TotalCost cost) {
            if (gateway_) {
                gateway_->replace_if_cheaper(gateway_id, cost);
                return;
            }

            gateway_ = FoundGateway{gateway_id, cost};

            auto now = time.now();
            auto elapsed = now - start_;
            timeout_ = nb::Delay{now, elapsed * DISCOVERY_BETTER_RESPONSE_TIMEOUT_RATE};
        }
    };

    class DiscoveryRequests {
        tl::Vec<DiscoveryEntry, MAX_CONCURRENT_DISCOVERIES> entries_;
        nb::Debounce debounce_;

      public:
        explicit DiscoveryRequests(util::Time &time) : debounce_{time, DISCOVER_INTERVAL} {}

        inline bool contains(const node::Destination &destination) const {
            return etl::any_of(entries_.begin(), entries_.end(), [&](const DiscoveryEntry &entry) {
                return entry.destination() == destination;
            });
        }

        inline nb::Poll<void> poll_addable() const {
            return entries_.full() ? nb::pending : nb::ready();
        }

        inline nb::Poll<void> add(const node::Destination &destination, util::Time &time) {
            if (contains(destination)) {
                return nb::ready();
            }
            if (entries_.full()) {
                return nb::pending;
            }
            entries_.emplace_back(destination, time.now());
            return nb::ready();
        }

        void on_gateway_found(
            util::Time &time,
            const node::Destination &destination,
            const node::NodeId &gateway_id,
            TotalCost cost
        ) {
            for (auto &entry : entries_) {
                if (entry.destination() == destination) {
                    entry.on_gateway_found(time, gateway_id, cost);
                    return;
                }
            }
        }

        void execute(util::Time &time) {
            if (debounce_.poll(time).is_pending()) {
                return;
            }

            util::Instant now = time.now();
            uint8_t i = 0;
            while (i < entries_.size()) {
                if (entries_[i].is_expired(now)) {
                    entries_.swap_remove(i);
                    continue;
                }
                i++;
            }
        }
    };

    class DiscoveryHandler {
        enum class State : uint8_t {
            Initial,
            RequestDiscovery,
            Discovering,
        } state_{State::Initial};

        node::Destination destination_;
        etl::optional<nb::Delay> timeout_;

      public:
        explicit DiscoveryHandler(const node::Destination &target_id) : destination_{target_id} {}

        // TODO: 引数大杉
        nb::Poll<etl::optional<node::NodeId>> execute(
            const local::LocalNodeService &lns,
            neighbor::NeighborService &ns,
            DiscoveryRequests &discovery,
            DiscoveryCache &discover_cache,
            TaskExecutor &task_executor,
            util::Time &time,
            util::Rand &rand
        ) {
            if (state_ == State::Initial) {
                if (destination_.is_broadcast()) {
                    return etl::optional(node::NodeId::broadcast());
                }

                const auto &node_id = destination_.node_id;
                if (ns.has_neighbor(node_id)) {
                    return etl::optional(node_id);
                }

                auto cache = discover_cache.get(destination_);
                if (cache) {
                    return etl::optional(cache->get().gateway_id);
                }

                if (discovery.contains(destination_)) {
                    state_ = State::Discovering;
                    return nb::pending;
                }

                state_ = State::RequestDiscovery;
            }

            if (state_ == State::RequestDiscovery) {
                POLL_UNWRAP_OR_RETURN(discovery.poll_addable());
                const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
                POLL_UNWRAP_OR_RETURN(
                    task_executor.request_send_discovery_frame(destination_, info, rand)
                );
                discovery.add(destination_, time);
                state_ = State::Discovering;
            }

            if (state_ == State::Discovering) {
                if (discovery.contains(destination_)) {
                    return nb::pending;
                }

                auto gateway_id = discover_cache.get(destination_);
                if (gateway_id) {
                    return etl::optional(gateway_id->get().gateway_id);
                } else {
                    LOG_INFO("Discovery failed: ", destination_);
                    return etl::optional<node::NodeId>();
                }
            }

            return nb::pending; // unreachable
        }
    };
} // namespace net::discovery
