#pragma once

#include <EthernetUdp.h>

#include "../address/udp.h"
#include "./constants.h"
#include "./util.h"

namespace media::ethernet {
    class FrameSender {
        memory::Static<net::link::FrameBroker> &broker_;
        etl::optional<net::frame::FrameBufferReader> reader_;

      public:
        explicit FrameSender(memory::Static<net::link::FrameBroker> &broker) : broker_{broker} {}

        void execute(EthernetUDP &udp) {
            while (!reader_.has_value()) {
                auto &&poll_frame =
                    broker_->poll_get_send_requested_frame(net::link::AddressType::Udp);
                if (poll_frame.is_pending()) {
                    return;
                }

                auto &&frame = poll_frame.unwrap();
                if (!UdpAddress::is_convertible_address(frame.remote)) {
                    continue;
                }

                const auto &address = UdpAddress(frame.remote);
                uint8_t begin_result =
                    udp.beginPacket(udp_address_to_ip(address), udp_address_to_port(address));
                if (begin_result != 1) {
                    continue;
                }

                udp.write(net::frame::protocol_number_to_byte(frame.protocol_number));
                reader_.emplace(etl::move(frame.reader));
            }

            uint8_t length = etl::min(MAX_TRANSFERRABLE_BYTES_AT_ONCE, reader_->readable_length());
            auto buffer = reader_->read_buffer_unchecked(length);
            udp.write(buffer.data(), length);

            if (reader_->is_all_read()) {
                reader_.reset();
                udp.endPacket();
            }
        }

        inline void on_link_down() {
            reader_.reset();
        }
    };
} // namespace media::ethernet
