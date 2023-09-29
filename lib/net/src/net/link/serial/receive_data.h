#pragma once

#include "../frame.h"
#include "./address.h"
#include "./layout.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    struct ReceiveDataHeader {
        SerialAddress destination;
        SerialAddress source;
        uint8_t length;
    };

    class ReceiveData {
        nb::stream::RepetitionCountWritableBuffer preamble_{PREAMBLE, PREAMBLE_LENGTH};
        nb::stream::FixedWritableBuffer<HEADER_LENGTH> header_;
        etl::optional<ReceiveDataHeader> header_parsed_;
        etl::optional<net::frame::FrameReception<Address>> reception_;

      public:
        ReceiveData() = default;
        ReceiveData(const ReceiveData &) = delete;
        ReceiveData(ReceiveData &&) = default;
        ReceiveData &operator=(const ReceiveData &) = delete;
        ReceiveData &operator=(ReceiveData &&) = default;

        template <net::frame::IFrameService<Address> FrameService>
        nb::Poll<void> execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (!header_parsed_.has_value()) {
                POLL_UNWRAP_OR_RETURN(preamble_.write_all_from(stream));
                POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));

                nb::buf::BufferSplitter splitter{header_.written_bytes()};
                header_parsed_ = ReceiveDataHeader{
                    .destination = splitter.parse<SerialAddressParser>(),
                    .source = splitter.parse<SerialAddressParser>(),
                    .length = splitter.split_1byte(),
                };
                DEBUG_ASSERT(splitter.is_empty(), "Invalid header size");
            }

            if (!reception_.has_value()) {
                uint8_t length = header_parsed_->length;
                reception_ = POLL_MOVE_UNWRAP_OR_RETURN(service.notify_reception(length));
                reception_.value().source.set_value(Address{header_parsed_->source});
            }

            return reception_->writer.write_all_from(stream);
        }
    };
} // namespace net::link::serial
