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

        inline constexpr AddressTypeSet unicast_supported_address_types() const {
            AddressTypeSet set;
            for (const auto &port : ports_) {
                set |= port->unicast_supported_address_types();
            }
            return set;
        }

        inline constexpr AddressTypeSet broadcast_supported_address_types() const {
            AddressTypeSet set;
            for (const auto &port : ports_) {
                set |= port->broadcast_supported_address_types();
            }
            return set;
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
        frame::ProtocolNumber protocol_number_;

      public:
        LinkSocket() = delete;
        LinkSocket(const LinkSocket &) = delete;
        LinkSocket(LinkSocket &&) = default;
        LinkSocket &operator=(const LinkSocket &) = delete;
        LinkSocket &operator=(LinkSocket &&) = delete;

        explicit LinkSocket(
            memory::StaticRef<LinkFrameQueue> queue,
            LinkPorts ports,
            frame::ProtocolNumber protocol_number
        )
            : queue_{queue},
              ports_{ports},
              protocol_number_{protocol_number} {}

        inline AddressTypeSet unicast_supported_address_types() const {
            return ports_.unicast_supported_address_types();
        }

        inline AddressTypeSet broadcast_supported_address_types() const {
            return ports_.broadcast_supported_address_types();
        }

        inline nb::Poll<LinkFrame> poll_receive_frame() {
            return queue_.get().poll_receive_frame(protocol_number_);
        }

        inline etl::expected<nb::Poll<void>, SendFrameError>
        poll_send_frame(const LinkAddress &remote, frame::FrameBufferReader &&reader) {
            auto type = remote.address_type();
            if (!ports_.unicast_supported_address_types().test(type)) {
                return etl::expected<nb::Poll<void>, SendFrameError>(
                    etl::unexpect, SendFrameError::SupportedMediaNotFound
                );
            }

            if (remote.is_broadcast() && !ports_.broadcast_supported_address_types().test(type)) {
                return etl::expected<nb::Poll<void>, SendFrameError>(
                    etl::unexpect, SendFrameError::BroadcastNotSupported
                );
            }

            return queue_.get().poll_request_send_frame(LinkFrame{
                .protocol_number = protocol_number_,
                .remote = remote,
                .reader = etl::move(reader),
            });
        }

        inline constexpr uint8_t max_payload_length() const {
            return frame::MTU;
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_frame_writer(frame::FrameService &frame_service, uint8_t payload_length) {
            return frame_service.request_frame_writer(payload_length);
        }

        inline nb::Poll<frame::FrameBufferWriter>
        poll_max_length_frame_writer(frame::FrameService &frame_service) {
            return frame_service.request_max_length_frame_writer();
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
            return LinkSocket{queue_, ports_, protocol_number};
        }

        inline AddressTypeSet unicast_supported_address_types() const {
            return ports_.unicast_supported_address_types();
        }

        inline AddressTypeSet broadcast_supported_address_types() const {
            return ports_.broadcast_supported_address_types();
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
