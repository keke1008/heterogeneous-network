#pragma once

#include <EthernetUdp.h>

#include "../address/udp.h"
#include "./constants.h"
#include "./util.h"
#include <net.h>

namespace media::ethernet {
    class FrameReceiver {
        net::link::FrameBroker broker_;

        struct ReceivingFrame {
            net::frame::ProtocolNumber protocol_number;
            net::link::Address address;
            net::frame::FrameBufferWriter writer;
        };

        etl::optional<ReceivingFrame> frame_;

      public:
        FrameReceiver() = delete;
        FrameReceiver(const FrameReceiver &) = delete;
        FrameReceiver(FrameReceiver &&) = delete;
        FrameReceiver &operator=(const FrameReceiver &) = delete;
        FrameReceiver &operator=(FrameReceiver &&) = delete;

        explicit FrameReceiver(const net::link::FrameBroker &broker) : broker_{etl::move(broker)} {}

        void execute(net::frame::FrameService &fs, EthernetUDP &udp, util::Time &time) {
            if (!frame_.has_value()) {
                int total_length = udp.parsePacket();
                if (total_length == 0) {
                    return;
                }

                // このネットワークにおけるUDPパケットのサイズとしておかしい場合は無視する
                int body_length = total_length - net::frame::PROTOCOL_SIZE;
                if (body_length < 0 || body_length > net::frame::MTU) {
                    return;
                }

                // 正しくないプロトコル番号の場合は無視する
                uint8_t protocol_byte = udp.read();
                auto opt_protocol = net::frame::byte_to_protocol_number(protocol_byte);
                if (!opt_protocol.has_value()) {
                    return;
                }

                // 受信バッファを取得できない場合はフレームを捨てる
                uint8_t length = static_cast<uint8_t>(body_length);
                auto &&poll_writer = fs.request_frame_writer(length);
                if (poll_writer.is_pending()) {
                    LOG_INFO(FLASH_STRING("Ethernet: no writer, discard frame"));
                    return;
                }

                UdpAddress &&remote = ip_and_port_to_udp_address(udp.remoteIP(), udp.remotePort());
                frame_ = ReceivingFrame{
                    .protocol_number = opt_protocol.value(),
                    .address = net::link::Address(remote),
                    .writer = etl::move(poll_writer.unwrap())
                };
            }

            auto &frame = frame_.value();
            if (!frame.writer.is_all_written()) {
                uint8_t buffer_writable_length = frame.writer.writable_length();
                uint8_t length = etl::min(MAX_TRANSFERRABLE_BYTES_AT_ONCE, buffer_writable_length);
                auto buffer = frame.writer.write_buffer_unchecked(length);
                udp.read(buffer.data(), length);

                if (!frame.writer.is_all_written()) {
                    return;
                }
            }

            auto poll = broker_.poll_dispatch_received_frame(
                frame.protocol_number, frame.address, frame.writer.create_reader(), time
            );
            if (poll.is_pending()) {
                LOG_INFO(FLASH_STRING("Ethernet: Failed to dispatch received frame"));
            }

            frame_.reset();
        }

        inline void on_link_down() {
            frame_.reset();
        }
    };
} // namespace media::ethernet
