#pragma once

#include "./facade.h"
#include <net/frame/service.h>
#include <net/socket.h>
#include <util/concepts.h>
#include <util/time.h>

namespace net::link {
    class MediaHandle {
        uint8_t index_;

      public:
        explicit MediaHandle(uint8_t index) : index_{index} {}

        inline uint8_t index() const {
            return index_;
        }
    };

    struct AddressEntry {
        MediaHandle handle;
        Address address;
    };

    template <frame::IFrameService FrameService, IMedia<FrameService> Media>
    class Socket;

    template <typename Service, uint8_t MEDIA_COUNT>
    concept IMediaService = requires(
        Service &service,
        typename Service::Address &address,
        MediaHandle handle,
        frame::ProtocolNumber protocol_number
    ) {
        { service.wait_for_media_detection() } -> util::same_as<nb::Poll<void>>;
        { service.get_addresses() } -> util::same_as<etl::vector<AddressEntry, MEDIA_COUNT>>;
        {
            service.make_socket(handle, protocol_number, address)
        } -> util::same_as<Socket<typename Service::FrameService, typename Service::Media>>;
    };

    template <frame::IFrameService FrameService, IMedia<FrameService> Media, uint8_t MEDIA_COUNT>
    class MediaService {
        const etl::span<Media, MEDIA_COUNT> media_;
        FrameService &frame_service_;

      public:
        using Address = typename FrameService::Address;

        MediaService() = delete;
        MediaService(const MediaService &) = delete;
        MediaService(MediaService &&) = delete;
        MediaService &operator=(const MediaService &) = delete;
        MediaService &operator=(MediaService &&) = delete;

        MediaService(FrameService &frame_service, etl::span<Media, MEDIA_COUNT> media)
            : frame_service_{frame_service},
              media_{media} {}

        inline nb::Poll<void> wait_for_media_detection() {
            for (auto &media : media_) {
                POLL_UNWRAP_OR_RETURN(media.wait_for_media_detection());
            }
            return nb::ready();
        }

        etl::vector<AddressEntry, MEDIA_COUNT> get_addresses() const {
            etl::vector<AddressEntry, MEDIA_COUNT> addresses;
            for (uint8_t i = 0; i < MEDIA_COUNT; i++) {
                auto opt_address = media_[i].get_address();
                if (opt_address.has_value()) {
                    addresses.emplace_back(MediaHandle{i}, opt_address.value());
                }
            }
            return addresses;
        }

        Socket<FrameService, Media> make_socket(
            MediaHandle target,
            frame::ProtocolNumber protocol_number,
            const Address &peer
        ) {
            return Socket<FrameService, Media>{
                frame_service_,
                media_[target.index()],
                protocol_number,
                peer,
            };
        }
    };

    template <frame::IFrameService FrameService, IMedia<FrameService> Media, uint8_t MEDIA_COUNT>
    MediaService(FrameService &, etl::span<Media, MEDIA_COUNT>)
        -> MediaService<FrameService, Media, MEDIA_COUNT>;

    template <frame::IFrameService FrameService, IMedia<FrameService> Media>
    class Socket {
        using Address = typename FrameService::Address;

        FrameService &frame_service_;
        Media &media_;
        frame::ProtocolNumber protocol_number_;
        Address peer_;

      public:
        explicit Socket(
            FrameService &frame_service_,
            Media &media,
            frame::ProtocolNumber protocol_number,
            Address peer
        )
            : frame_service_{frame_service_},
              media_{media},
              protocol_number_{protocol_number},
              peer_{peer} {}

        inline frame::ProtocolNumber protocol_number() const {
            return protocol_number_;
        }

        inline const Address &peer() const {
            return peer_;
        }

        inline nb::Poll<frame::FrameBufferWriter> request_frame_writer(uint8_t length) {
            return frame_service_.request_frame_writer(length);
        }

        inline nb::Poll<frame::FrameBufferWriter> request_max_length_frame_writer() {
            return frame_service_.request_max_length_frame_writer();
        }

        inline nb::Poll<void> send_frame(frame::FrameBufferReader &&reader) {
            return media_.send_frame(Frame{
                .protocol_number = protocol_number_,
                .peer = peer_,
                .length = reader.frame_length(),
                .reader = etl::move(reader),
            });
        }

        inline nb::Poll<frame::FrameBufferReader> receive_frame() {
            return media_.receive_frame().reader;
        }
    };
} // namespace net::link
