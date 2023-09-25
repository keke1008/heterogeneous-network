#pragma once

#include "../address.h"
#include "../frame.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/stream.h>

namespace net::link::serial {
    class ReceiveData {
        nb::stream::FixedWritableBuffer<SerialAddress::SIZE + frame::BODY_LENGTH_SIZE> header_;
        nb::Promise<DataReader> body_reader_;
        etl::optional<nb::Future<void>> barrier_;

      public:
        explicit ReceiveData(nb::Promise<DataReader> &&body_reader)
            : body_reader_{etl::move(body_reader)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            if (!barrier_.has_value()) {
                POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));

                nb::buf::BufferSplitter splitter{header_.written_bytes()};
                SerialAddress address = splitter.parse<SerialAddressParser>();
                uint8_t body_length = splitter.split_1byte();
                DEBUG_ASSERT(splitter.is_empty(), "Invalid header size");

                auto [future, promise] = nb::make_future_promise_pair<void>();
                barrier_ = etl::move(future);
                DataReader reader{etl::ref(stream), etl::move(promise), body_length};
                body_reader_.set_value(etl::move(reader));
            }

            return barrier_.value().poll();
        }
    };
} // namespace net::link::serial
