#pragma once

#include <net/frame/service.h>

namespace test {
    template <uint8_t LENGTH>
    etl::pair<memory::RcPoolCounter, net::frame::FrameBuffer<LENGTH>> make_frame_buffer() {
        memory::RcPoolCounter counter;
        net::frame::FrameBuffer<LENGTH> buffer{LENGTH};
        return {etl::move(counter), etl::move(buffer)};
    }

    template <typename Address, uint8_t LENGTH>
    etl::pair<::net::frame::FrameTransmission, net::frame::FrameTransmissionRequest<Address>>
    make_frame_transmission_request(
        uint8_t protocol,
        const Address &destination,
        memory::RcPoolCounter &counter,
        net::frame::FrameBuffer<LENGTH> &buffer
    ) {
        auto buffer_ref = net::frame::FrameBufferReference{&counter, &buffer};
        auto [reader, writer] = net::frame::make_frame_buffer_pair(etl::move(buffer_ref));
        auto [future, promise] = nb::make_future_promise_pair<bool>();
        return {
            net::frame::FrameTransmission{etl::move(writer), etl::move(future)},
            net::frame::FrameTransmissionRequest<Address>{
                protocol,
                destination,
                etl::move(reader),
                etl::move(promise),
            },
        };
    }

    template <typename Address, uint8_t LENGTH>
    etl::pair<net::frame::FrameReception<Address>, net::frame::FrameReceptionNotification<Address>>
    make_frame_reception_notification(
        memory::RcPoolCounter &counter,
        nb::stream::FixedReadableWritableBuffer<LENGTH> &buffer
    ) {
        auto buffer_ref = net::frame::FrameBufferReference{&counter, &buffer};
        auto [reader, writer] = net::frame::make_frame_buffer_pair(etl::move(buffer_ref));
        auto [future, promise] = nb::make_future_promise_pair<Address>();
        return {
            net::frame::FrameReception<Address>{etl::move(writer), etl::move(promise)},
            net::frame::FrameReceptionNotification<Address>{etl::move(reader), etl::move(future)},
        };
    }
} // namespace test
