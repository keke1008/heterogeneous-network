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

    template <nb::AsyncReadableWritable RW>
    class LinkPorts {
        etl::vector<memory::Static<MediaPort<RW>>, link::MAX_MEDIA_PORT> &ports_;

      public:
        LinkPorts() = delete;
        LinkPorts(const LinkPorts &) = default;
        LinkPorts(LinkPorts &&) = default;
        LinkPorts &operator=(const LinkPorts &) = delete;
        LinkPorts &operator=(LinkPorts &&) = delete;

        explicit LinkPorts(etl::vector<memory::Static<MediaPort<RW>>, MAX_MEDIA_PORT> &ports)
            : ports_{ports} {
            FASSERT(ports.size() <= MAX_MEDIA_PORT);
        }

        inline etl::optional<Address> get_broadcast_address(AddressType type) const {
            switch (type) {
            case AddressType::UHF:
                return Address(uhf::ModemId::broadcast());
            default:
                return etl::nullopt;
            }
        }

        inline AddressTypeSet unicast_supported_address_types() const {
            AddressTypeSet set;
            for (const auto &port : ports_) {
                set |= port->unicast_supported_address_types();
            }
            return set;
        }

        inline AddressTypeSet broadcast_supported_address_types() const {
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

        inline etl::optional<Address> get_media_address() const {
            for (const auto &port : ports_) {
                const auto &info = port->get_media_info();
                if (info.address.has_value()) {
                    return info.address.value();
                }
            }
            return etl::nullopt;
        }

        inline void get_media_addresses(etl::vector<Address, MAX_MEDIA_PORT> &dest) const {
            for (const auto &port : ports_) {
                const auto &info = port->get_media_info();
                if (info.address.has_value()) {
                    dest.push_back(info.address.value());
                }
            }
        }

        inline etl::optional<etl::reference_wrapper<memory::Static<MediaPort<RW>>>>
        get_port(MediaPortNumber port) {
            uint8_t index = port.value();
            return index < ports_.size() ? etl::optional(etl::ref(ports_[index])) : etl::nullopt;
        }

        inline etl::optional<etl::reference_wrapper<const memory::Static<MediaPort<RW>>>>
        get_port(MediaPortNumber port) const {
            uint8_t index = port.value();
            return index < ports_.size() ? etl::optional(etl::cref(ports_[index])) : etl::nullopt;
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

    template <nb::AsyncReadableWritable RW>
    class LinkSocket {
        memory::Static<LinkFrameQueue> &queue_;
        LinkPorts<RW> ports_;
        frame::ProtocolNumber protocol_number_;

      public:
        LinkSocket() = delete;
        LinkSocket(const LinkSocket &) = delete;
        LinkSocket(LinkSocket &&) = default;
        LinkSocket &operator=(const LinkSocket &) = delete;
        LinkSocket &operator=(LinkSocket &&) = delete;

        explicit LinkSocket(
            memory::Static<LinkFrameQueue> &queue,
            LinkPorts<RW> ports,
            frame::ProtocolNumber protocol_number
        )
            : queue_{queue},
              ports_{ports},
              protocol_number_{protocol_number} {}

        inline etl::optional<Address> get_broadcast_address(AddressType type) const {
            return ports_.get_broadcast_address(type);
        }

        inline AddressTypeSet unicast_supported_address_types() const {
            return ports_.unicast_supported_address_types();
        }

        inline AddressTypeSet broadcast_supported_address_types() const {
            return ports_.broadcast_supported_address_types();
        }

        inline nb::Poll<LinkReceivedFrame> poll_receive_frame() {
            return queue_.get().poll_receive_frame(protocol_number_);
        }

        inline etl::expected<nb::Poll<void>, SendFrameError> poll_send_frame(
            const Address &remote,
            frame::FrameBufferReader &&reader,
            etl::optional<MediaPortNumber> port,
            util::Time &time
        ) {
            auto type = remote.type();
            if (!ports_.unicast_supported_address_types().test(type)) {
                return etl::expected<nb::Poll<void>, SendFrameError>(
                    etl::unexpect, SendFrameError::SupportedMediaNotFound
                );
            }

            return queue_.get().poll_request_send_frame(
                protocol_number_, remote, etl::move(reader), port, time
            );
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

    template <nb::AsyncReadableWritable RW>
    class LinkService {
        LinkPorts<RW> ports_;
        ProtocolLock lock_;
        memory::Static<LinkFrameQueue> &queue_;

      public:
        LinkService() = delete;
        LinkService(const LinkService &) = delete;
        LinkService(LinkService &&) = delete;
        LinkService &operator=(const LinkService &) = delete;
        LinkService &operator=(LinkService &&) = delete;

        explicit LinkService(
            etl::vector<memory::Static<MediaPort<RW>>, MAX_MEDIA_PORT> &ports,
            memory::Static<LinkFrameQueue> &queue
        )
            : ports_{ports},
              queue_{queue} {}

        inline etl::optional<Address> get_broadcast_address(AddressType type) const {
            return ports_.get_broadcast_address(type);
        }

        inline LinkSocket<RW> open(frame::ProtocolNumber protocol_number) {
            FASSERT(!lock_.is_locked(protocol_number));
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

        inline etl::optional<Address> get_media_address() const {
            return ports_.get_media_address();
        }

        inline void get_media_addresses(etl::vector<Address, MAX_MEDIA_PORT> &dest) const {
            ports_.get_media_addresses(dest);
        }

        inline etl::optional<etl::reference_wrapper<memory::Static<MediaPort<RW>>>>
        get_port(MediaPortNumber port) {
            return ports_.get_port(port);
        }

        inline const etl::optional<etl::reference_wrapper<const memory::Static<MediaPort<RW>>>>
        get_port(MediaPortNumber port) const {
            return ports_.get_port(port);
        }

        inline void
        execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            ports_.execute(frame_service, time, rand);
        }
    };
}; // namespace net::link
