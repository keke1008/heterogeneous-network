#pragma once

#include "./frame.h"
#include <memory/pair_shared.h>
#include <net/frame/service.h>
#include <stdint.h>
#include <util/concepts.h>

namespace net::stream {
    class StreamFrameSender {
        memory::Owned<etl::optional<frame::FrameBufferWriter>> writer_;

      public:
        StreamFrameSender() = delete;
        StreamFrameSender(const StreamFrameSender &) = delete;
        StreamFrameSender(StreamFrameSender &&) = default;
        StreamFrameSender &operator=(const StreamFrameSender &) = delete;
        StreamFrameSender &operator=(StreamFrameSender &&) = delete;

        explicit StreamFrameSender(memory::Owned<etl::optional<frame::FrameBufferWriter>> &&writer)
            : writer_{etl::move(writer)} {}

        inline bool is_closed() {
            return !writer_.has_pair() && writer_.get().value().is_buffer_filled();
        }

        // 送信されていないデータがある場合は`nb::Pending`を返す
        template <frame::IFrameBufferRequester Requester, frame::IFrameSender Sender>
        nb::Poll<void> execute(Requester &requester, Sender &sender) {
            if (writer_.get().has_value()) {
                if (writer_.get().value().is_buffer_filled()) {
                    POLL_UNWRAP_OR_RETURN(sender.send_frame(etl::move(writer_.get().value())));
                    writer_.get() = etl::nullopt;
                    return nb::ready();
                }
                return nb::pending;
            }

            if (writer_.has_pair()) {
                writer_.get().value() =
                    POLL_MOVE_UNWRAP_OR_RETURN(requester.request_max_length_frame_writer());
            }
            return nb::pending;
        }
    };

    static etl::pair<StreamFrameSender, StreamWriter> make_send_stream() {
        auto [owned, reference] =
            memory::make_shared<etl::optional<frame::FrameBufferWriter>>(etl::nullopt);
        return {StreamFrameSender{etl::move(owned)}, StreamWriter{etl::move(reference)}};
    }

    class StreamFrameReceiver {
        memory::Reference<etl::optional<frame::FrameBufferReader>> reader_;

      public:
        StreamFrameReceiver() = delete;
        StreamFrameReceiver(const StreamFrameReceiver &) = delete;
        StreamFrameReceiver(StreamFrameReceiver &&) = default;
        StreamFrameReceiver &operator=(const StreamFrameReceiver &) = delete;
        StreamFrameReceiver &operator=(StreamFrameReceiver &&) = delete;

        explicit StreamFrameReceiver(
            memory::Reference<etl::optional<frame::FrameBufferReader>> &&reader
        )
            : reader_{etl::move(reader)} {}

        template <frame::IFrameReceiver Receiver>
        void execute(Receiver &receiver) {
            if (!reader_.has_pair()) {      // 受信したデータを扱う相手がいない
                receiver.receive_frame()(); // 受信したデータを捨てる
                return;
            }

            auto &reader = reader_.get().value().get();
            if (!reader.has_value() || reader.value().is_all_read()) {
                auto frame = receiver.receive_frame();
                if (frame.is_ready()) {
                    reader = etl::move(frame.unwrap());
                }
            }
        }

        void close() {
            reader_.unpair();
        }
    };

    static etl::pair<StreamFrameReceiver, StreamReader> make_receive_stream() {
        auto [owned, reference] =
            memory::make_shared<etl::optional<frame::FrameBufferReader>>(etl::nullopt);
        return {StreamFrameReceiver{etl::move(reference)}, StreamReader{etl::move(owned)}};
    }
} // namespace net::stream
