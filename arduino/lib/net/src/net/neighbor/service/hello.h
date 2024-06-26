#pragma once

#include "../constants.h"
#include "./table.h"
#include "./task.h"
#include <net/local.h>

namespace net::neighbor::service {
    class SendHelloWorker {
        struct Interval {
            nb::Debounce debounce_;

            explicit Interval(util::Time &time) : debounce_{time, SEND_HELLO_INTERVAL} {}
        };

        struct Broadcast {
            link::AddressTypeIterator broadcast_sending_type_{};
            link::AddressTypeSet broadcast_sent_types_{};
        };

        struct UnicastPollCursor {
            link::AddressTypeSet broadcast_sent_types_;
        };

        struct Unicast {
            link::AddressTypeSet broadcast_sent_types_;
            NeighborListCursor cursor_;
        };

        etl::variant<Interval, Broadcast, UnicastPollCursor, Unicast> state_;

      public:
        explicit SendHelloWorker(util::Time &time) : state_{Interval{time}} {}

        template <uint8_t N>
        void execute(
            link::MediaService auto &ms,
            NeighborList &list,
            const local::LocalNodeService &lns,
            TaskExecutor<N> &executor,
            util::Time &time
        ) {
            const auto &poll_info = lns.poll_info();
            if (poll_info.is_pending()) {
                return;
            }
            const auto &info = poll_info.unwrap();

            if (etl::holds_alternative<Interval>(state_)) {
                if (etl::get<Interval>(state_).debounce_.poll(time).is_pending()) {
                    return;
                }

                auto config = lns.config();
                if (config.enable_auto_neighbor_discovery) {
                    state_.emplace<Broadcast>();
                } else {
                    state_.emplace<UnicastPollCursor>(link::AddressTypeSet{});
                }
            }

            if (etl::holds_alternative<Broadcast>(state_)) {
                auto &[send_type, sent_types] = etl::get<Broadcast>(state_);
                while (send_type != send_type.end()) {
                    auto type = *send_type;
                    const auto &opt_address = ms.get_broadcast_address(type);
                    if (!opt_address.has_value()) {
                        ++send_type;
                        continue;
                    }

                    const auto &address = opt_address.value();
                    auto poll = executor.poll_send_keep_alive(
                        info, link::MediaPortMask::unspecified(), address, node::Cost(0), type
                    );
                    if (poll.is_pending()) {
                        return;
                    }

                    sent_types.set(type);
                    ++send_type;
                }

                state_.emplace<UnicastPollCursor>(sent_types);
            }

            if (etl::holds_alternative<UnicastPollCursor>(state_)) {
                auto &&poll_cursor = list.poll_cursor();
                if (poll_cursor.is_pending()) {
                    return;
                }

                auto sent_ = etl::get<UnicastPollCursor>(state_).broadcast_sent_types_;
                state_.emplace<Unicast>(sent_, etl::move(poll_cursor.unwrap()));
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

                    auto &address = addresses.front();
                    auto poll = executor.poll_send_keep_alive(
                        info, address.gateway_port_mask, address.address, node::Cost(0),
                        neighbor.id()
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
