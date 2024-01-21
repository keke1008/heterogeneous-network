#pragma once

#include "./frame.h"
#include <etl/optional.h>
#include <nb/serde.h>
#include <net/frame.h>
#include <net/link.h>

namespace media::serial {
    struct SkipPreamble {
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &readable) {
        continue_outer:
            POLL_UNWRAP_OR_RETURN(readable.poll_readable(PREAMBLE_LENGTH + LAST_PREAMBLE_LENGTH));
            for (uint8_t i = 0; i < PREAMBLE_LENGTH; i++) {
                if (readable.read_unchecked() != PREAMBLE) {
                    goto continue_outer;
                }
            }

            for (uint8_t i = 0; i < LAST_PREAMBLE_LENGTH; i++) {
                if (readable.read_unchecked() != LAST_PREAMBLE) {
                    goto continue_outer;
                }
            }

            return nb::DeserializeResult::Ok;
        }
    };

    class ReceiveData {
        net::frame::FrameBufferWriter frame_writer_;

      public:
        inline explicit ReceiveData(net::frame::FrameBufferWriter &&frame_writer)
            : frame_writer_{etl::move(frame_writer)} {}

        template <nb::AsyncReadable R>
        inline nb::Poll<void> execute(R &readable) {
            while (!frame_writer_.is_all_written()) {
                POLL_UNWRAP_OR_RETURN(readable.poll_readable(1));
                frame_writer_.write_unchecked(readable.read_unchecked());
            }
            return nb::ready();
        }
    };

    class DiscardData {
        nb::de::SkipNBytes discarding_;

      public:
        inline explicit DiscardData(uint8_t length) : discarding_{length} {}

        template <nb::AsyncReadable R>
        inline nb::Poll<void> execute(R &reader) {
            POLL_UNWRAP_OR_RETURN(discarding_.deserialize(reader));
            return nb::ready();
        }
    };

    class FrameReceiver {
        memory::Static<net::link::FrameBroker> &broker_;
        etl::optional<SerialAddress> self_address_;
        etl::optional<SerialAddress> remote_address_;
        etl::variant<SkipPreamble, AsyncSerialFrameHeaderDeserializer, ReceiveData, DiscardData>
            state_{};

      public:
        explicit FrameReceiver(memory::Static<net::link::FrameBroker> &broker) : broker_{broker} {}

        inline etl::optional<SerialAddress> get_self_address() const {
            return self_address_;
        }

        inline etl::optional<SerialAddress> get_remote_address() const {
            return remote_address_;
        }

      private:
        void update_address_by_received_frame_header(const SerialFrameHeader &header) {
            // 最初に受信したフレームの送信元アドレスをリモートアドレスとする
            if (!remote_address_) {
                remote_address_ = header.source;
            }

            // 最初に受信したフレームの送信先アドレスを自アドレスとする
            if (!self_address_) {
                self_address_ = header.destination;
            }

            if (header.source != *remote_address_) {
                LOG_INFO(
                    "received frame from unknown address: ", net::link::Address(header.source)
                );
            }
        }

      public:
        template <nb::AsyncReadable R>
        void execute(net::frame::FrameService &fs, R &readable, util::Time &time) {
            if (etl::holds_alternative<SkipPreamble>(state_)) {
                if (etl::get<SkipPreamble>(state_).deserialize(readable).is_ready()) {
                    state_.emplace<AsyncSerialFrameHeaderDeserializer>();
                }
            }

            if (etl::holds_alternative<AsyncSerialFrameHeaderDeserializer>(state_)) {
                auto &state = etl::get<AsyncSerialFrameHeaderDeserializer>(state_);
                auto poll_result = state.deserialize(readable);
                if (poll_result.is_pending()) {
                    return;
                }
                if (poll_result.unwrap() != nb::DeserializeResult::Ok) {
                    state_ = SkipPreamble{};
                    return;
                }

                const auto &header = state.result();
                update_address_by_received_frame_header(header);

                if (header.destination != *self_address_ || header.source != *remote_address_) {
                    state_ = DiscardData{header.length};
                } else if (auto &&poll_writer = fs.request_frame_writer(header.length);
                           poll_writer.is_pending()) {
                    LOG_INFO(FLASH_STRING("Serial: no writer, discard frame"));
                    state_ = DiscardData{header.length};
                } else {
                    auto &&writer = poll_writer.unwrap();
                    auto poll = broker_->poll_dispatch_received_frame(
                        header.protocol_number, net::link::Address{header.source},
                        writer.create_reader(), time
                    );
                    if (poll.is_ready()) {
                        state_ = ReceiveData{etl::move(writer)};
                    } else {
                        state_ = DiscardData{header.length};
                    }
                }
            }

            if (etl::holds_alternative<ReceiveData>(state_)) {
                if (etl::get<ReceiveData>(state_).execute(readable).is_ready()) {
                    state_.emplace<SkipPreamble>();
                }
            }

            if (etl::holds_alternative<DiscardData>(state_)) {
                if (etl::get<DiscardData>(state_).execute(readable).is_ready()) {
                    state_.emplace<SkipPreamble>();
                }
            }
        }

        inline bool try_initialize_local_address(SerialAddress address) {
            if (self_address_) {
                return false;
            }
            self_address_ = address;
            return true;
        }
    };
} // namespace media::serial
