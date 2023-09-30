#pragma once

#include "./layout.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    struct ReceiveDataHeader {
        SerialAddress source;
        SerialAddress destination;
        uint8_t length;

        static ReceiveDataHeader parse(etl::span<const uint8_t> bytes) {
            nb::buf::BufferSplitter splitter{bytes};
            return ReceiveDataHeader{
                .source = splitter.parse<SerialAddressParser>(),
                .destination = splitter.parse<SerialAddressParser>(),
                .length = splitter.split_1byte(),
            };
            DEBUG_ASSERT(splitter.is_empty(), "Invalid header size");
        }
    };

    class ReceiveData {
        SerialAddress source_;

        nb::stream::RepetitionCountWritableBuffer preamble_{PREAMBLE, PREAMBLE_LENGTH};
        nb::stream::FixedWritableBuffer<HEADER_LENGTH> header_;
        etl::optional<ReceiveDataHeader> header_parsed_;
        etl::optional<net::frame::FrameReception<Address>> reception_;

        // 宛先が自分でない場合に，データを破棄するために使用する
        etl::optional<nb::stream::DiscardingCountWritableBuffer> discarding_;

      public:
        ReceiveData() = delete;
        ReceiveData(const ReceiveData &) = delete;
        ReceiveData(ReceiveData &&) = default;
        ReceiveData &operator=(const ReceiveData &) = delete;
        ReceiveData &operator=(ReceiveData &&) = default;

        explicit ReceiveData(SerialAddress source) : source_{source} {}

        template <net::frame::IFrameService<Address> FrameService>
        nb::Poll<void> execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (discarding_.has_value()) {
                return discarding_->write_all_from(stream);
            }

            if (!header_parsed_.has_value()) {
                POLL_UNWRAP_OR_RETURN(preamble_.write_all_from(stream));
                POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
                header_parsed_ = ReceiveDataHeader::parse(header_.written_bytes());

                // 宛先が自分でない場合はデータを破棄する
                if (header_parsed_->destination != source_) {
                    discarding_ = nb::stream::DiscardingCountWritableBuffer{header_parsed_->length};
                    return discarding_->write_all_from(stream);
                }
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
