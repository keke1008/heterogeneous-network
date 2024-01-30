#pragma once

#include "./constants.h"
#include "./frame.h"
#include <nb/poll.h>
#include <net/notification.h>
#include <net/routing.h>

namespace net::observer {
    class NotificationService {
        etl::optional<frame::FrameBufferReader> sending_buffer_;

      public:
        void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            notification::NotificationService &ns,
            routing::RoutingSocket<FRAME_DELAY_POOL_SIZE> &socket,
            util::Time &time,
            util::Rand &rand,
            etl::optional<etl::reference_wrapper<const node::Destination>> observer
        ) {
            if (!observer.has_value()) {
                return;
            }

            while (true) {
                if (sending_buffer_.has_value()) {
                    auto &buffer = sending_buffer_.value();
                    auto poll_send = socket.poll_send_frame(observer->get(), etl::move(buffer));
                    if (poll_send.is_pending()) {
                        return;
                    }

                    sending_buffer_.reset();
                }

                auto metadata = get_frame_metadata(ns);
                if (metadata.serialized_length == 0) {
                    return;
                }

                const auto &dest = observer->get();
                auto &&poll_writer =
                    socket.poll_frame_writer(fs, lns, rand, dest, metadata.serialized_length);
                if (poll_writer.is_pending()) {
                    return;
                }

                auto &&writer = poll_writer.unwrap();
                serialize_frame(writer, metadata, ns);
                ns.pop(metadata.entry_count);

                sending_buffer_.emplace(writer.create_reader());
            }
        }
    };
} // namespace net::observer
