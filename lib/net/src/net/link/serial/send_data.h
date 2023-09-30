#pragma once

#include "../address.h"
#include "./address.h"
#include "./layout.h"
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class SendData {
        nb::stream::RepetitionReadableBuffer preamble_{PREAMBLE, PREAMBLE_LENGTH};
        net::frame::FrameTransmissionRequest<Address> request_;
        nb::stream::FixedReadableBuffer<HEADER_LENGTH> header_;

      public:
        SendData() = delete;
        SendData(const SendData &) = delete;
        SendData(SendData &&) = default;
        SendData &operator=(const SendData &) = delete;
        SendData &operator=(SendData &&) = default;

        explicit SendData(
            net::frame::FrameTransmissionRequest<Address> &&request,
            SerialAddress source
        )
            : request_{etl::move(request)},
              header_{
                  source,
                  SerialAddress{request_.destination},
                  nb::buf::FormatBinary(request_.reader.frame_length()),
              } {}

        inline nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(preamble_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(header_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(request_.reader.read_all_into(stream));
            request_.success.set_value(true);
            return nb::ready();
        }
    };
} // namespace net::link::serial
