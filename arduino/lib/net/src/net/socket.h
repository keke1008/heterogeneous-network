#pragma once

#include <nb/poll.h>
#include <net/frame/buffer.h>
#include <util/concepts.h>

namespace net::socket {
    template <typename SenderSocker>
    concept ISenderSocket =
        requires(SenderSocker &socket, uint8_t frame_length, frame::FrameBufferReader &reader) {
            {
                socket.requset_frame_writer(frame_length)
            } -> util::same_as<nb::Poll<frame::FrameBufferWriter>>;

            {
                socket.request_max_length_frame_writer()
            } -> util::same_as<nb::Poll<frame::FrameBufferWriter>>;

            // readyが返る場合は，writerの所有権は奪われる
            { socket.send_frame(etl::move(reader)) } -> util::same_as<nb::Poll<void>>;

            // ソケット固有の処理を実行する
            // socketがcloseした場合はreadyを返す．
            { socket.execute() } -> util::same_as<nb::Poll<void>>;
        };

    template <typename ReceiverSocket>
    concept IReceiverSocket = requires(ReceiverSocket &socket) {
        { socket.receive_frame() } -> util::same_as<nb::Poll<frame::FrameBufferReader>>;

        // ソケット固有のの処理を実行する
        // socketがcloseした場合はreadyを返す．
        { socket.execute() } -> util::same_as<nb::Poll<void>>;
    };
} // namespace net::socket
