#pragma once

#include "./node.h"
#include "./service.h"
#include <debug_assert.h>
#include <nb/barrier.h>

namespace net::routing {
    struct FrameHeader {
        NodeId source_id;
        NodeId destination_id;
    };

    class AsyncFrameHeaderParser {
        AsyncNodeIdParser source_id_parser_;
        AsyncNodeIdParser destination_id_parser_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            POLL_UNWRAP_OR_RETURN(source_id_parser_.parse(splitter));
            return destination_id_parser_.parse(splitter);
        }

        inline FrameHeader result() {
            return FrameHeader{
                .source_id = source_id_parser_.result(),
                .destination_id = destination_id_parser_.result(),
            };
        }
    };

    struct Frame {
        NodeId source_id;
        NodeId destination_id;
        frame::FrameBufferReader reader;

        Frame(const FrameHeader &header, frame::FrameBufferReader &&reader)
            : source_id{header.source_id},
              destination_id{header.destination_id},
              reader{etl::move(reader)} {}
    };

    class ReceiveFrameTask {
        struct ParsingFrame {
            frame::FrameBufferReader reader;
            AsyncFrameHeaderParser parser;
        };

        etl::variant<etl::monostate, ParsingFrame, Frame> received_frame_;

      public:
        ReceiveFrameTask() {}

        explicit ReceiveFrameTask(Frame &&received_frame)
            : received_frame_{etl::move(received_frame)} {}

        inline bool is_receivable() const {
            return etl::holds_alternative<etl::monostate>(received_frame_);
        }

        nb::Poll<etl::reference_wrapper<Frame>> get_frame_ref() {
            if (etl::holds_alternative<Frame>(received_frame_)) {
                auto &frame = etl::get<Frame>(received_frame_);
                return nb::ready(etl::ref(frame));
            } else {
                return nb::pending;
            }
        }

        nb::Poll<Frame> take_frame() {
            if (etl::holds_alternative<Frame>(received_frame_)) {
                auto frame = etl::move(etl::get<Frame>(received_frame_));
                received_frame_ = etl::monostate{};
                return etl::move(frame);
            } else {
                return nb::pending;
            }
        }

        void execute(link::LinkService &link_service_, frame::ProtocolNumber protocol_number_) {
            if (etl::holds_alternative<etl::monostate>(received_frame_)) {
                auto poll_frame = link_service_.receive_frame(protocol_number_);
                if (poll_frame.is_pending()) {
                    return;
                }
                received_frame_ = ParsingFrame{
                    .reader = etl::move(poll_frame.unwrap().reader),
                    .parser = AsyncFrameHeaderParser{},
                };
            }

            if (etl::holds_alternative<ParsingFrame>(received_frame_)) {
                auto &frame = etl::get<ParsingFrame>(received_frame_);
                if (frame.reader.read(frame.parser).is_pending()) {
                    return;
                }
                received_frame_ = Frame{frame.parser.result(), etl::move(frame.reader)};
            }
        }
    };

    template <frame::IFrameService FrameService>
    class Socket {
        FrameService &frame_service_;
        link::LinkService &link_service_;
        RoutingService &routing_service_;

        etl::optional<nb::BarrierController> barrier_controller_;

        ReceiveFrameTask receive_frame_task_;
        frame::ProtocolNumber protocol_number_;
        NodeId destination_id_;

        inline void write_header(frame::FrameBufferWriter &writer) {
            writer.write(routing_service_.self_id(), destination_id_);
        }

      public:
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;
        Socket(Socket &&) = default;
        Socket &operator=(Socket &&) = delete;

        explicit Socket(
            FrameService &frame_service,
            link::LinkService &link_service,
            RoutingService &routing_service,
            frame::ProtocolNumber protocol_number,
            const NodeId &peer_id
        )
            : frame_service_{frame_service},
              link_service_{link_service},
              routing_service_{routing_service},
              protocol_number_{protocol_number},
              destination_id_{peer_id} {}

        explicit Socket(
            FrameService &frame_service,
            link::LinkService &link_service,
            RoutingService &routing_service,
            nb::BarrierController &&barrier_controller,
            Frame &&received_frame_,
            frame::ProtocolNumber protocol_number,
            const NodeId &peer_id
        )
            : frame_service_{frame_service},
              link_service_{link_service},
              routing_service_{routing_service},
              barrier_controller_{etl::move(barrier_controller)},
              receive_frame_task_{etl::move(received_frame_)},
              protocol_number_{protocol_number},
              destination_id_{peer_id} {}

        inline uint8_t max_frame_length() const {
            return frame::MTU - routing_service_.self_id().serialized_length() -
                destination_id_.serialized_length();
        }

        inline nb::Poll<frame::FrameBufferWriter> request_frame_writer(uint8_t length) {
            DEBUG_ASSERT(length <= max_frame_length());
            uint8_t total_length = length + routing_service_.self_id().serialized_length() +
                destination_id_.serialized_length();
            auto frame = POLL_MOVE_UNWRAP_OR_RETURN(frame_service_.request_frame_writer(length));
            write_header(frame);
            return etl::move(frame);
        }

        inline nb::Poll<frame::FrameBufferWriter> request_max_length_frame_writer() {
            auto frame =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service_.request_max_length_frame_writer());
            write_header(frame);
            return etl::move(frame);
        }

        enum class SendResult : uint8_t {
            Success,
            Unreachable,
        };

        nb::Poll<SendResult> send_frame(frame::FrameBufferReader &&frame) {
            auto address = routing_service_.resolve_gateway_address(destination_id_);
            if (!address.has_value()) {
                return nb::ready(SendResult::Unreachable);
            }

            POLL_UNWRAP_OR_RETURN(
                link_service_.send_frame(protocol_number_, address.value(), etl::move(frame))
            );
            return nb::ready(SendResult::Success);
        }

        inline nb::Poll<Frame> receive_frame() {
            auto &frame = POLL_UNWRAP_OR_RETURN(receive_frame_task_.get_frame_ref()).get();
            if (frame.destination_id == routing_service_.self_id()) {
                return receive_frame_task_.take_frame();
            } else {
                return nb::pending;
            }
        }

        void execute() {
            receive_frame_task_.execute(link_service_, protocol_number_);
            auto frame_ref = receive_frame_task_.get_frame_ref();
            if (frame_ref.is_pending() ||
                frame_ref.unwrap().get().destination_id == routing_service_.self_id()) {
                return;
            }

            auto &frame = frame_ref.unwrap().get();
            auto gateway_address = routing_service_.resolve_gateway_address(frame.destination_id);
            if (!gateway_address.has_value()) {
                return;
            }
            auto poll = link_service_.send_frame(
                protocol_number_, gateway_address.value(), frame.reader.make_initial_clone()
            );
            if (poll.is_ready()) {
                receive_frame_task_.take_frame();
            }
        }
    };

    template <typename FrameService>
    class Acceptor {
        frame::ProtocolNumber protocol_number_;
        nb::Barrier barrier_;
        ReceiveFrameTask receive_frame_task_;

      public:
        explicit Acceptor(frame::ProtocolNumber protocol_number)
            : protocol_number_{protocol_number},
              barrier_{nb::Barrier::dangling()} {}

        nb::Poll<Socket<FrameService>> poll_accept(
            FrameService &frame_service,
            link::LinkService &link_service,
            RoutingService &routing_service
        ) {
            POLL_UNWRAP_OR_RETURN(barrier_.poll_wait());
            receive_frame_task_.execute(link_service, protocol_number_);
            auto frame = POLL_MOVE_UNWRAP_OR_RETURN(receive_frame_task_.take_frame());
            return Socket<FrameService>{
                frame_service,
                link_service,
                routing_service,
                etl::move(barrier_.make_controller().value()),
                etl::move(frame),
                protocol_number_,
                routing_service.self_id(),
            };
        }
    };
} // namespace net::routing
