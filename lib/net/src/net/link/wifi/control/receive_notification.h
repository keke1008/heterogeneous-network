#pragma once

#include "../link.h"
#include "../message/receive_data.h"
#include <debug_assert.h>
#include <nb/future.h>
#include <nb/stream.h>
#include <serde/dec.h>
#include <util/span.h>

namespace net::link::wifi {
    class ReceiveNotification {
        nb::stream::SingleLineWritableBuffer<19> buffer_; // longest: +IPD,0,2147483647\r\n
        nb::Promise<ReceiveDataMessage> promise_;

      public:
        explicit ReceiveNotification(nb::Promise<ReceiveDataMessage> &&promise)
            : promise_{etl::move(promise)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream));
            auto data = buffer_.poll().unwrap();

            util::span::split_until_byte_exclusive(data, ','); // drop +IPD

            auto link_id_span = util::span::split_until_byte_exclusive(data, ',');
            DEBUG_ASSERT(link_id_span.has_value());
            auto link_id = serde::dec::deserialize<LinkId>(link_id_span.value());

            auto length_span = util::span::split_until_byte_exclusive(data, '\r');
            DEBUG_ASSERT(length_span.has_value());
            auto length = serde::dec::deserialize<uint32_t>(length_span.value());

            promise_.set_value(ReceiveDataMessage{link_id, length});
            return nb::ready();
        }
    };
} // namespace net::link::wifi
