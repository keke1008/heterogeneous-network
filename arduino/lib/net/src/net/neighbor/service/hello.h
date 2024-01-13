#pragma once

#include "../constants.h"
#include "../table.h"
#include "./task.h"
#include <net/local.h>

namespace net::neighbor::service {
    class SendHelloWorker {
        struct Interval {
            nb::Debounce debounce_;

            explicit Interval(util::Time &time) : debounce_{time, SEND_HELLO_INTERVAL} {}
        };

        struct Broadcast {
            link::AddressTypeSet broadcast_sending_types_;
            link::AddressTypeSet broadcast_reachable_types_;

            explicit Broadcast(link::AddressTypeSet broadcast_types)
                : broadcast_sending_types_{broadcast_types},
                  broadcast_reachable_types_{broadcast_types} {}
        };

        struct UnicastPollCursor {
            link::AddressTypeSet broadcast_reached_types_;
        };

        struct Unicast {
            link::AddressTypeSet broadcast_reached_types_;
            NeighborListCursor cursor_;
        };

        etl::variant<Interval, Broadcast, UnicastPollCursor, Unicast> state_;

      public:
        explicit SendHelloWorker(util::Time &time) : state_{Interval{time}} {}

        template <nb::AsyncReadableWritable RW, uint8_t N>
        void execute(
            NeighborList &list,
            frame::FrameService &fs,
            link::LinkService<RW> &ls,
            notification::NotificationService &nts,
            const local::LocalNodeInfo &info,
            TaskExecutor<RW, N> &executor,
            util::Time &time
        ) {
            if (etl::holds_alternative<Interval>(state_)) {
                if (etl::get<Interval>(state_).debounce_.poll(time).is_pending()) {
                    return;
                }

                if (info.config.enable_auto_neighbor_discovery) {
                    state_.emplace<Broadcast>(ls.broadcast_supported_address_types());
                } else {
                    state_.emplace<UnicastPollCursor>(link::AddressTypeSet{});
                }
            }

            if (etl::holds_alternative<Broadcast>(state_)) {
                auto &state = etl::get<Broadcast>(state_);
                while (state.broadcast_sending_types_.any()) {
                    auto type = *state.broadcast_sending_types_.pick();
                    const auto &opt_address = ls.get_broadcast_address(type);
                    if (!opt_address.has_value()) {
                        state.broadcast_sending_types_.reset(type);
                        continue;
                    }

                    const auto &address = opt_address.value();
                    auto poll = executor.poll_send_keep_alive(info, address, node::Cost(0), type);
                    if (poll.is_pending()) {
                        return;
                    }

                    state.broadcast_sending_types_.reset(type);
                }

                state_.emplace<UnicastPollCursor>(state.broadcast_reachable_types_);
            }

            if (etl::holds_alternative<UnicastPollCursor>(state_)) {
                auto &&poll_cursor = list.poll_cursor();
                if (poll_cursor.is_pending()) {
                    return;
                }

                auto reached = etl::get<UnicastPollCursor>(state_).broadcast_reached_types_;
                state_.emplace<Unicast>(reached, etl::move(poll_cursor.unwrap()));
            }

            if (etl::holds_alternative<Unicast>(state_)) {
                while (true) {
                    auto &state = etl::get<Unicast>(state_);
                    etl::optional<etl::reference_wrapper<const NeighborNode>> opt_neighbor =
                        list.get_neighbor_node(state.cursor_);
                    if (!opt_neighbor.has_value()) {
                        state_.emplace<Interval>(Interval{time});
                        return;
                    }

                    const auto &neighbor = opt_neighbor.value().get();
                    if (!neighbor.should_send_hello(time)) {
                        state.cursor_.advance();
                        continue;
                    }

                    auto addresses = neighbor.addresses();
                    if (addresses.empty()) {
                        state.cursor_.advance();
                        continue;
                    }

                    auto poll = executor.poll_send_keep_alive(
                        info, addresses.front(), node::Cost(0), neighbor.id()
                    );
                    if (poll.is_pending()) {
                        return;
                    }

                    state.cursor_.advance();
                }

                state_.emplace<Interval>(Interval{time});
            }
        }
    };
} // namespace net::neighbor::service
