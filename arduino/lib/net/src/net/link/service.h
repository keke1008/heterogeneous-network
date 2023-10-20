#pragma once

#include "./facade.h"
#include "./media.h"
#include <nb/poll.h>

namespace net::link {
    class LinkService {
        etl::span<MediaFacade> media_;

      public:
        LinkService() = delete;
        LinkService(const LinkService &) = delete;
        LinkService(LinkService &&) = delete;
        LinkService &operator=(const LinkService &) = delete;
        LinkService &operator=(LinkService &&) = delete;

        explicit LinkService(etl::span<MediaFacade> media) : media_{media} {}

        nb::Poll<void> wait_for_initialization() {
            for (auto &media : media_) {
                POLL_UNWRAP_OR_RETURN(media.wait_for_media_detection());
            }
            return nb::ready();
        }

        bool is_supported_address(const Address &address) {
            return etl::any_of(media_.begin(), media_.end(), [&](MediaFacade &media) {
                return media.is_supported_address_type(address.type());
            });
        }

        uint8_t get_addresses(etl::span<Address> dest) {
            uint8_t count = 0;
            for (auto &media : media_) {
                auto address = media.get_address();
                if (address.has_value()) {
                    dest[count] = address.value();
                    count++;
                }
            }
            return count;
        }

        nb::Poll<void> send_frame(
            frame::ProtocolNumber protocol_number,
            Address &peer,
            frame::FrameBufferReader &&reader
        ) {
            SendingFrame frame{
                .protocol_number = protocol_number,
                .peer = peer,
                .reader_ref = etl::move(reader),
            };

            for (auto &media : media_) {
                if (!media.is_supported_address_type(peer.type())) {
                    continue;
                }
                if (media.send_frame(frame).is_ready()) {
                    return nb::ready();
                }
            }

            return nb::pending;
        }

        nb::Poll<ReceivedFrame> receive_frame(frame::ProtocolNumber protocol_number) {
            for (auto &media : media_) {
                auto poll_frame = media.receive_frame(protocol_number);
                if (poll_frame.is_ready()) {
                    auto &frame = poll_frame.unwrap();
                    return ReceivedFrame{frame.peer, etl::move(frame.reader)};
                }
            }

            return nb::pending;
        }

        template <frame::IFrameService FrameService>
        void execute(FrameService &frame_service) {
            for (auto &media : media_) {
                media.execute(frame_service);
            }
        }

        using JoinApResult = etl::expected<nb::Poll<nb::Future<bool>>, UnSupportedOperation>;

        inline JoinApResult
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            bool is_supported = false;
            for (auto &media : media_) {
                auto &&result = media.join_ap(ssid, password);
                if (!result.has_value()) {
                    continue;
                }

                is_supported = true;
                if (result.value().is_ready()) {
                    return etl::move(result);
                }
            }

            return is_supported ? JoinApResult{nb::pending} : JoinApResult{etl::unexpect};
        }
    };
}; // namespace net::link
