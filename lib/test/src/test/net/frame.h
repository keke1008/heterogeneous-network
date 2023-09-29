#pragma once

#include <net/frame/service.h>

namespace test {
    template <uint8_t LENGTH>
    etl::pair<memory::RcPoolCounter, nb::stream::FixedReadableWritableBuffer<LENGTH>>
    make_frame_buffer() {
        memory::RcPoolCounter counter;
        nb::stream::FixedReadableWritableBuffer<LENGTH> buffer;
        return {etl::move(counter), etl::move(buffer)};
    }

    template <typename Address, uint8_t LENGTH>
    etl::pair<::net::frame::FrameTransmission, net::frame::FrameTransmissionRequest<Address>>
    make_frame_transmission_request(
        const Address &destination,
        memory::RcPoolCounter &counter,
        nb::stream::FixedReadableWritableBuffer<LENGTH> &buffer
    ) {
        auto buffer_ref = net::frame::FrameBufferReference{&counter, &buffer};
        auto [reader, writer] = net::frame::make_frame_buffer_pair(etl::move(buffer_ref));
        auto [future, promise] = nb::make_future_promise_pair<bool>();
        return {
            net::frame::FrameTransmission{etl::move(writer), etl::move(future)},
            net::frame::FrameTransmissionRequest<Address>{
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
