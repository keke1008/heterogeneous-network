#pragma once

#include "./frame.h"

namespace net::observer {
    class SyncService {
        struct Write {
            NodeSyncFrameWriter writer;
        };

        struct Send {
            frame::FrameBufferReader reader;
        };

        etl::variant<etl::monostate, Write, Send> state_;
        nb::Debounce sync_debounce_;

      public:
        explicit SyncService(util::Time &time) : sync_debounce_{time, NODE_SYNC_INTERVAL} {}

        void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborService &ns,
            routing::RoutingSocket<FRAME_DELAY_POOL_SIZE> &socket,
            etl::optional<etl::reference_wrapper<const node::Destination>> observer,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!observer.has_value()) {
                return;
            }

            if (sync_debounce_.poll(time).is_pending()) {
                return;
            }

            if (etl::holds_alternative<etl::monostate>(state_)) {
                state_.emplace<Write>();
            }

            if (etl::holds_alternative<Write>(state_)) {
                auto &writer = etl::get<Write>(state_).writer;
                auto &&poll_writer = writer.execute(fs, lns, ns, socket, observer->get(), rand);
                if (poll_writer.is_pending()) {
                    return;
                }

                state_.emplace<Send>(etl::move(poll_writer.unwrap()));
            }

            if (etl::holds_alternative<Send>(state_)) {
                auto &reader = etl::get<Send>(state_).reader;
                auto poll_send = socket.poll_send_frame(observer->get(), etl::move(reader));
                if (poll_send.is_pending()) {
                    return;
                }

                state_.emplace<etl::monostate>();
            }
        }
    };
} // namespace net::observer
