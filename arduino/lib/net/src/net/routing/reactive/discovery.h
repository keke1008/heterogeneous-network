#pragma once

#include "../node.h"
#include "./cache.h"
#include "./constants.h"
#include "./task.h"
#include <nb/time.h>
#include <tl/vec.h>

namespace net::routing::reactive {
    struct FoundRoute {
        NodeId gateway_id;
        Cost cost;

        void replace_if_cheaper(const NodeId &gateway_id_, Cost cost_) {
            if (cost_ < cost) {
                gateway_id = gateway_id_;
                cost = cost_;
            }
        }
    };

    class DiscoveryEntry {
        NodeId remote_id_;
        util::Instant start_;
        etl::optional<FoundRoute> route_{etl::nullopt};

      public:
        DiscoveryEntry(const NodeId &remote_id, util::Instant start)
            : remote_id_{remote_id},
              start_{start} {}

        inline const NodeId &remote_id() const {
            return remote_id_;
        }

        inline const etl::optional<FoundRoute> &get_route() const {
            return route_;
        }

        inline bool is_expired(util::Instant now) const {
            auto elapsed = now - start_;

            if (route_.has_value()) { // 少なくとも一度はルートを見つけている場合
                return elapsed >= DISCOVERY_BETTER_RESPONSE_TIMEOUT;
            } else { // ルートを見つけていない場合
                return elapsed >= DISCOVERY_FIRST_RESPONSE_TIMEOUT;
            }
        }

        inline void replace_if_cheaper(const NodeId &gateway_id, Cost cost) {
            if (!route_) {
                route_ = FoundRoute{gateway_id, cost};
            } else {
                route_->replace_if_cheaper(gateway_id, cost);
            }
        }
    };

    class Discovery {
        tl::Vec<DiscoveryEntry, MAX_CONCURRENT_DISCOVERIES> entries_;
        nb::Debounce debounce_;

      public:
        explicit Discovery(util::Time &time) : debounce_{time, DISCOVER_INTERVAL} {}

        inline bool contains(const NodeId &id) const {
            return etl::any_of(entries_.begin(), entries_.end(), [&](const DiscoveryEntry &entry) {
                return entry.remote_id() == id;
            });
        }

        inline nb::Poll<void> poll_addable() const {
            return entries_.full() ? nb::pending : nb::ready();
        }

        inline nb::Poll<void> add(const NodeId &id, util::Time &time) {
            if (contains(id)) {
                return nb::ready();
            }
            if (entries_.full()) {
                return nb::pending;
            }
            entries_.emplace_back(id, time.now());
            return nb::ready();
        }

        void on_route_found(const NodeId &remote_id, const NodeId &gateway_id, Cost cost) {
            for (auto &entry : entries_) {
                if (entry.remote_id() == remote_id) {
                    entry.replace_if_cheaper(gateway_id, cost);
                    return;
                }
            }
        }

        void execute(util::Time &time, RouteCache &route_cache) {
            if (debounce_.poll(time).is_pending()) {
                return;
            }

            util::Instant now = time.now();

            for (uint8_t i = 0; i < entries_.size(); i++) {
                auto &entry = entries_[i];
                if (entry.is_expired(now)) {
                    auto &route = entry.get_route();
                    if (route.has_value()) {
                        route_cache.add(entry.remote_id(), route->gateway_id);
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
            Discovering,
        } state_{State::Initial};

        NodeId target_id_;

      public:
        explicit DiscoveryHandler(const NodeId &target_id) : target_id_{target_id} {}

        // TODO: 引数大杉
        nb::Poll<etl::optional<NodeId>> execute(
            Discovery &discovery,
            RouteCache &route_cache,
            TaskExecutor &task_executor,
            const NodeId &self_id,
            const Cost &self_cost,
            util::Time &time,
            util::Rand &rand
        ) {
            if (state_ == State::Initial) {
                auto gateway_id = route_cache.get(target_id_);
                if (gateway_id) {
                    return etl::optional(gateway_id->get());
                }

                if (discovery.contains(target_id_)) {
                    state_ = State::Discovering;
                    return nb::pending;
                }

                POLL_UNWRAP_OR_RETURN(discovery.poll_addable());
                POLL_UNWRAP_OR_RETURN(
                    task_executor.request_send_discovery_frame(target_id_, self_id, self_cost, rand)
                );
                discovery.add(target_id_, time);
                state_ = State::Discovering;
            }

            if (state_ == State::Discovering) {
                if (discovery.contains(target_id_)) {
                    return nb::pending;
                }

                auto gateway_id = route_cache.get(target_id_);
                if (gateway_id) {
                    return etl::optional(gateway_id->get());
                } else {
                    return etl::optional<NodeId>();
                }
            }

            return nb::pending; // unreachable
        }
    };
} // namespace net::routing::reactive
