#pragma once

#include "../packet.h"

namespace net::trusted::common {
    template <PacketType Type>
    class CreateControlPacketTask {
      public:
        template <frame::IFrameBufferRequester Requester>
        inline nb::Poll<frame::FrameBufferReader> execute(Requester &requester) {
            uint8_t length = frame_length<Type>();
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(requester.request_frame_writer(length));
            writer.build(PacketHeaderWriter{Type});
            return writer.make_initial_reader();
        }
    };

    class SendPacketTask {
        frame::FrameBufferReader reader_;

      public:
        inline explicit SendPacketTask(frame::FrameBufferReader &&reader)
            : reader_{etl::move(reader)} {}

        template <frame::IFrameBufferRequester Requester, frame::IFrameSender Sender>
        inline nb::Poll<frame::FrameBufferReader> execute(Requester &requester, Sender &sender) {
            POLL_UNWRAP_OR_RETURN(sender.send_frame(etl::move(reader_.make_initial_clone())));
            return etl::move(reader_);
        }
    };

    class TimeoutError {};

    class WaitingForReceivingPacketTask {
        util::Instant timeout_;

      public:
        inline explicit WaitingForReceivingPacketTask(util::Instant timeout) : timeout_{timeout} {}

        using Result = etl::expected<frame::FrameBufferReader, TimeoutError>;

        template <frame::IFrameReceiver Receiver>
        inline nb::Poll<Result> execute(Receiver &receiver, util::Time &time) {
            if (time.now() >= timeout_) {
                return Result{etl::unexpected<TimeoutError>{TimeoutError{}}};
            }

            auto reader = POLL_MOVE_UNWRAP_OR_RETURN(receiver.receive_frame());
            return Result{etl::move(reader)};
        }
    };

    class ParsePacketTypeTask {
        frame::FrameBufferReader receive_reader_;

      public:
        inline explicit ParsePacketTypeTask(frame::FrameBufferReader &&receive_reader)
            : receive_reader_{etl::move(receive_reader)} {}

        template <frame::IFrameReceiver Receiver>
        inline nb::Poll<etl::pair<PacketType, frame::FrameBufferReader>>
        execute(Receiver &receiver, util::Time &time) {
            if (receive_reader_.readable_count() < 1) {
                return nb::pending;
            }

            PacketType type = static_cast<PacketType>(receive_reader_.read());
            return etl::pair<PacketType, frame::FrameBufferReader>{
                type,
                etl::move(receive_reader_),
            };
        }
    };

    class ReceivePacketTask {
        etl::variant<WaitingForReceivingPacketTask, ParsePacketTypeTask> state_;

      public:
        ReceivePacketTask(const util::Instant &timeout)
            : state_{WaitingForReceivingPacketTask{timeout}} {}

        using Result = etl::expected<etl::pair<PacketType, frame::FrameBufferReader>, TimeoutError>;

        template <frame::IFrameReceiver Receiver>
        nb::Poll<Result> execute(Receiver &receiver, util::Time &time) {
            if (etl::holds_alternative<WaitingForReceivingPacketTask>(state_)) {
                auto &state = etl::get<WaitingForReceivingPacketTask>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(receiver, time));
                if (!result.has_value()) {
                    return Result(etl::unexpected<TimeoutError>{TimeoutError{}});
                }

                auto &reader = result.value();
                state_ = ParsePacketTypeTask{etl::move(reader)};
            }

            if (etl::holds_alternative<ParsePacketTypeTask>(state_)) {
                auto &state = etl::get<ParsePacketTypeTask>(state_);
                return etl::expected(POLL_MOVE_UNWRAP_OR_RETURN(state.execute(receiver)));
            }

            return nb::pending;
        }
    };
} // namespace net::trusted::common
