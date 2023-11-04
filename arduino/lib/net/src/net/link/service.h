#pragma once

#include "./broker.h"
#include "./media.h"
#include "./port.h"
#include <etl/bitset.h>
#include <nb/poll.h>

namespace net::link {
    class ProtocolLock {
        etl::bitset<frame::NUM_PROTOCOLS> locked_protocol_;

      public:
        inline bool is_locked(frame::ProtocolNumber protocol_number) {
            uint8_t i = static_cast<uint8_t>(protocol_number);
            return locked_protocol_.test(i);
        }

        inline void lock(frame::ProtocolNumber protocol_number) {
            uint8_t i = static_cast<uint8_t>(protocol_number);
            locked_protocol_.set(i);
        }
    };

    constexpr uint8_t MAX_MEDIA_PORT = 4;

    class LinkPorts {
        etl::span<memory::StaticRef<MediaPort>> ports_;

      public:
        LinkPorts() = delete;
        LinkPorts(const LinkPorts &) = default;
        LinkPorts(LinkPorts &&) = default;
        LinkPorts &operator=(const LinkPorts &) = default;
        LinkPorts &operator=(LinkPorts &&) = default;

        explicit LinkPorts(etl::span<memory::StaticRef<MediaPort>> ports) : ports_{ports} {
            DEBUG_ASSERT(ports.size() <= MAX_MEDIA_PORT);
        }

        bool is_unicast_supported(AddressType type) const {
            return etl::any_of(
                ports_.begin(), ports_.end(),
                [&](memory::StaticRef<MediaPort> &port) {
                    return port->is_unicast_supported(type).unwrap_or(false);
                }
            );
        }

        bool is_broadcast_supported(AddressType type) const {
            return etl::any_of(
                ports_.begin(), ports_.end(),
                [&](memory::StaticRef<MediaPort> &port) {
                    return port->is_broadcast_supported(type).unwrap_or(false);
                }
            );
        }

        inline void get_media_info(etl::span<etl::optional<MediaInfo>, MAX_MEDIA_PORT> dest) const {
            for (uint8_t i = 0; i < ports_.size(); ++i) {
                dest[i] = ports_[i]->get_media_info();
            }
        }

        inline memory::StaticRef<MediaPort> get_port(uint8_t index) const {
            DEBUG_ASSERT(index < ports_.size());
            return ports_[index];
        }

        inline void
        execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            for (auto &media : ports_) {
                media->execute(frame_service, time, rand);
            }
        }
    };

    enum class SendFrameError {
        SupportedMediaNotFound,
        BroadcastNotSupported,
    };

    class LinkSocket {
        memory::StaticRef<LinkFrameQueue> queue_;
        LinkPorts ports_;

      public:
        LinkSocket() = delete;
        LinkSocket(const LinkSocket &) = delete;
        LinkSocket(LinkSocket &&) = default;
        LinkSocket &operator=(const LinkSocket &) = delete;
        LinkSocket &operator=(LinkSocket &&) = delete;

        explicit LinkSocket(memory::StaticRef<LinkFrameQueue> queue, LinkPorts ports)
            : queue_{queue},
              ports_{ports} {}

        inline nb::Poll<LinkFrame> poll_receive_frame(frame::ProtocolNumber protocol_number) {
            return queue_.get().poll_receive_frame(protocol_number);
        }

        inline etl::expected<nb::Poll<void>, SendFrameError> poll_send_frame(LinkFrame &&frame) {
            if (!ports_.is_unicast_supported(frame.remote.address_type())) {
                return etl::expected<nb::Poll<void>, SendFrameError>(
                    etl::unexpect, SendFrameError::SupportedMediaNotFound
                );
            }

            if (frame.remote.is_broadcast() &&
                !ports_.is_broadcast_supported(frame.remote.address_type())) {
                return etl::expected<nb::Poll<void>, SendFrameError>(
                    etl::unexpect, SendFrameError::BroadcastNotSupported
                );
            }

            return queue_.get().poll_request_send_frame(etl::move(frame));
        }
    };

    class LinkService {
        LinkPorts ports_;
        ProtocolLock lock_;
        memory::StaticRef<LinkFrameQueue> queue_;

      public:
        LinkService() = delete;
        LinkService(const LinkService &) = delete;
        LinkService(LinkService &&) = delete;
        LinkService &operator=(const LinkService &) = delete;
        LinkService &operator=(LinkService &&) = delete;

        explicit LinkService(
            etl::span<memory::StaticRef<MediaPort>> ports,
            memory::StaticRef<LinkFrameQueue> queue
        )
            : ports_{ports},
              queue_{queue} {}

        inline LinkSocket open(frame::ProtocolNumber protocol_number) {
            DEBUG_ASSERT(!lock_.is_locked(protocol_number));
            lock_.lock(protocol_number);
            return LinkSocket{queue_, ports_};
        }

        inline void get_media_info(etl::span<etl::optional<MediaInfo>, MAX_MEDIA_PORT> dest) const {
            ports_.get_media_info(dest);
        }

        inline memory::StaticRef<MediaPort> get_port(uint8_t index) const {
            return ports_.get_port(index);
        }

        inline void
        execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            ports_.execute(frame_service, time, rand);
        }
    };
}; // namespace net::link
