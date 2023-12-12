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
        node::Cost cost;

        void replace_if_cheaper(const node::NodeId &gateway_id_, node::Cost cost_) {
            if (cost_ < cost) {
                gateway_id = gateway_id_;
                cost = cost_;
            }
        }
    };

    class DiscoveryEntry {
        node::NodeId remote_id_;
        util::Instant start_;
        etl::optional<FoundGateway> gateway_{etl::nullopt};

      public:
        DiscoveryEntry(const node::NodeId &remote_id, util::Instant start)
            : remote_id_{remote_id},
              start_{start} {}

        inline const node::NodeId &remote_id() const {
            return remote_id_;
        }

        inline const etl::optional<FoundGateway> &get_gateway() const {
            return gateway_;
        }

        inline bool is_expired(util::Instant now) const {
            auto elapsed = now - start_;

            if (gateway_.has_value()) { // 少なくとも一度はルートを見つけている場合
                return elapsed >= DISCOVERY_BETTER_RESPONSE_TIMEOUT;
            } else { // ルートを見つけていない場合
                return elapsed >= DISCOVERY_FIRST_RESPONSE_TIMEOUT;
            }
        }

        inline void replace_if_cheaper(const node::NodeId &gateway_id, node::Cost cost) {
            if (!gateway_) {
                gateway_ = FoundGateway{gateway_id, cost};
            } else {
                gateway_->replace_if_cheaper(gateway_id, cost);
            }
        }
    };

    class DiscoveryRequests {
        tl::Vec<DiscoveryEntry, MAX_CONCURRENT_DISCOVERIES> entries_;
        nb::Debounce debounce_;

      public:
        explicit DiscoveryRequests(util::Time &time) : debounce_{time, DISCOVER_INTERVAL} {}

        inline bool contains(const node::NodeId &id) const {
            return etl::any_of(entries_.begin(), entries_.end(), [&](const DiscoveryEntry &entry) {
                return entry.remote_id() == id;
            });
        }

        inline nb::Poll<void> poll_addable() const {
            return entries_.full() ? nb::pending : nb::ready();
        }

        inline nb::Poll<void> add(const node::NodeId &id, util::Time &time) {
            if (contains(id)) {
                return nb::ready();
            }
            if (entries_.full()) {
                return nb::pending;
            }
            entries_.emplace_back(id, time.now());
            return nb::ready();
        }

        void on_gateway_found(
            const node::NodeId &remote_id,
            const node::NodeId &gateway_id,
            node::Cost cost
        ) {
            for (auto &entry : entries_) {
                if (entry.remote_id() == remote_id) {
                    entry.replace_if_cheaper(gateway_id, cost);
                    return;
                }
            }
        }

        void execute(util::Time &time, DiscoveryCache &discovery_cache) {
            if (debounce_.poll(time).is_pending()) {
                return;
            }

            util::Instant now = time.now();

            for (uint8_t i = 0; i < entries_.size(); i++) {
                auto &entry = entries_[i];
                if (entry.is_expired(now)) {
                    auto &gateway = entry.get_gateway();
                    if (gateway.has_value()) {
                        discovery_cache.add(entry.remote_id(), gateway->gateway_id);
                    }

                    entries_.swap_remove(i);
                    i--;
                }
            }
        }
    };

    class DiscoveryHandler {
        enum class State : uint8_t {
            Initial,
            RequestDiscovery,
            Discovering,
        } state_{State::Initial};

        node::NodeId target_id_;

      public:
        explicit DiscoveryHandler(const node::NodeId &target_id) : target_id_{target_id} {}

        // TODO: 引数大杉
        template <nb::AsyncReadableWritable RW>
        nb::Poll<etl::optional<node::NodeId>> execute(
            const node::LocalNodeService &local_node_service,
            neighbor::NeighborService<RW> &neighbor_service,
            DiscoveryRequests &discovery,
            DiscoveryCache &discover_cache,
            TaskExecutor<RW> &task_executor,
            util::Time &time,
            util::Rand &rand
        ) {
            if (state_ == State::Initial) {
                if (neighbor_service.has_neighbor(target_id_)) {
                    return etl::optional(target_id_);
                }

                auto gateway_id = discover_cache.get(target_id_);
                if (gateway_id) {
                    return etl::optional(gateway_id->get());
                }

                if (discovery.contains(target_id_)) {
                    state_ = State::Discovering;
                    return nb::pending;
                }

                state_ = State::RequestDiscovery;
            }

            if (state_ == State::RequestDiscovery) {
                POLL_UNWRAP_OR_RETURN(discovery.poll_addable());
                const auto &info = POLL_UNWRAP_OR_RETURN(local_node_service.poll_info());
                POLL_UNWRAP_OR_RETURN(
                    task_executor.request_send_discovery_frame(target_id_, info.id, info.cost, rand)
                );
                discovery.add(target_id_, time);
                state_ = State::Discovering;
            }

            if (state_ == State::Discovering) {
                if (discovery.contains(target_id_)) {
                    return nb::pending;
                }

                auto gateway_id = discover_cache.get(target_id_);
                if (gateway_id) {
                    return etl::optional(gateway_id->get());
                } else {
                    return etl::optional<node::NodeId>();
                }
            }

            return nb::pending; // unreachable
        }
    };
} // namespace net::discovery
