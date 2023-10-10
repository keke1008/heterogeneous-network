#pragma once

#include <etl/expected.h>
#include <nb/poll.h>
#include <net/frame/service.h>
#include <util/time.h>

namespace net::trusted {
    enum class TrustedError : uint8_t {
        ConnectionRefused,
        Timeout,
        BadNetwork,
    };

    enum class PacketType : uint8_t {
        SYN,
        ACK,
        NACK,
        DATA,
        FIN,
    };

    template <PacketType Type>
    constexpr uint8_t frame_length() {
        static_assert(Type != PacketType::DATA, "DATA packet type is not supported");
        return 1;
    }

    class PacketHeaderWriter {
        PacketType type_;

      public:
        explicit PacketHeaderWriter(PacketType type) : type_{type} {}

        inline void write_to_buffer(nb::buf::BufferBuilder &builder) const {
            builder.append(static_cast<uint8_t>(type_));
        }
    };

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

    class WaitingForReceivingPacketTask {
        util::Instant timeout_;

      public:
        inline explicit WaitingForReceivingPacketTask(util::Instant timeout) : timeout_{timeout} {}

        class TimeoutError {};

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
        inline nb::Poll<PacketType> execute(Receiver &receiver, util::Time &time) {
            if (receive_reader_.readable_count() < 1) {
                return nb::pending;
            }
            return static_cast<PacketType>(receive_reader_.read());
        }
    };
} // namespace net::trusted
