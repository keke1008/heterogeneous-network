#pragma once

#include "../frame.h"
#include "./address.h"
#include <etl/optional.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class SendData {
        net::frame::FrameTransmissionRequest<Address> request_;
        nb::stream::FixedReadableBuffer<SerialAddress::SIZE + frame::BODY_LENGTH_SIZE> header_;

      public:
        SendData() = delete;
        SendData(const SendData &) = delete;
        SendData(SendData &&) = default;
        SendData &operator=(const SendData &) = delete;
        SendData &operator=(SendData &&) = default;

        explicit SendData(net::frame::FrameTransmissionRequest<Address> &&request)
            : request_{etl::move(request)},
              header_{request.destination, request.reader.frame_length()} {}

        inline nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(header_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(request_.reader.read_all_into(stream));
            request_.success.set_value(true);
            return nb::ready();
        }
    };
} // namespace net::link::serial
