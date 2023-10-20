#pragma once

#include "./address.h"
#include <net/frame/service.h>
#include <stdint.h>
#include <util/time.h>

namespace net::link {
    enum class MediaType : uint8_t {
        UHF,
        Wifi,
        Serial,
    };

    struct Frame {
        frame::ProtocolNumber protocol_number;
        Address peer;
        uint8_t length;
        frame::FrameBufferReader reader;
    };

    struct SendingFrame {
        frame::ProtocolNumber protocol_number;
        Address peer;
        frame::FrameBufferReader &&reader_ref;
    };

    struct ReceivedFrame {
        Address peer;
        frame::FrameBufferReader reader;
    };

    template <typename Media, typename FrameService>
    concept IMedia = frame::IFrameService<FrameService> &&
        requires(Media &media, FrameService &frame_service, Frame &frame) {
            { media.wait_for_media_detection() } -> util::same_as<nb::Poll<void>>;
            { media.get_address() } -> util::same_as<etl::optional<typename FrameService::Address>>;
            { media.send_frame(etl::move(frame)) } -> util::same_as<nb::Poll<void>>;
            { media.receive_frame() } -> util::same_as<nb::Poll<Frame>>;
            { media.execute(frame_service) } -> util::same_as<void>;
        };
} // namespace net::link
