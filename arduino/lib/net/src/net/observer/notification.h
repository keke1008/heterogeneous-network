#pragma once

#include "./constants.h"
#include "./frame.h"
#include <nb/poll.h>
#include <net/notification.h>
#include <net/routing.h>

namespace net::observer {
    class NotificationService {
        struct Write {
            NodeNotificationFrameWriter writer;
        };

        struct Send {
            frame::FrameBufferReader reader;
        };

        etl::variant<etl::monostate, Write, Send> state_;

      public:
        void execute(
            frame::FrameService &fs,
            notification::NotificationService &nts,
            const local::LocalNodeService &lns,
            routing::RoutingSocket<FRAME_DELAY_POOL_SIZE> &socket,
            util::Time &time,
            util::Rand &rand,
            etl::optional<etl::reference_wrapper<const node::Destination>> observer
        ) {
            if (!observer.has_value()) {
                return;
            }

            if (etl::holds_alternative<etl::monostate>(state_)) {
                if (nts.size() == 0) {
                    return;
                }

                state_.emplace<Write>();
            }

            if (etl::holds_alternative<Write>(state_)) {
                auto &writer = etl::get<Write>(state_).writer;
                auto &&poll_writer = writer.execute(fs, nts, lns, socket, observer->get(), rand);
                if (poll_writer.is_pending()) {
                    return;
                }

                nts.clear();
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
