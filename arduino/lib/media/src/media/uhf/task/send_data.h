#pragma once

#include "../common.h"
#include "../response.h"
#include "./carrier_sence.h"
#include "./fixed.h"
#include "./interruption.h"
#include <nb/serde.h>
#include <nb/time.h>
#include <net/frame.h>

namespace media::uhf {
    class AsyncSendDataCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer prefix_{"@DT"};
        nb::ser::Hex<uint8_t> length_;
        net::frame::AsyncProtocolNumberSerializer protocol_;
        net::frame::AsyncFrameBufferReaderSerializer payload_;
        nb::ser::AsyncStaticSpanSerializer route_prefix_{"/R"};
        AsyncModemIdSerializer destination_;
        nb::ser::AsyncStaticSpanSerializer suffix_{"\r\n"};

      public:
        explicit AsyncSendDataCommandSerializer(UhfFrame &&frame)
            : length_{static_cast<uint8_t>(frame.reader.origin_length() + net::frame::PROTOCOL_SIZE)},
              protocol_{frame.protocol_number},
              payload_{frame.reader.origin()},
              destination_{frame.remote} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            POLL_UNWRAP_OR_RETURN(prefix_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(length_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(protocol_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(payload_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(route_prefix_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(destination_.serialize(writable));
            return suffix_.serialize(writable);
        }

        uint8_t serialized_length() const {
            return prefix_.serialized_length() + length_.serialized_length() +
                protocol_.serialized_length() + payload_.serialized_length() +
                route_prefix_.serialized_length() + destination_.serialized_length() +
                suffix_.serialized_length();
        }
    };

    enum class InformationResponseResult : uint8_t {
        Ok,
        Error,
    };

    template <nb::AsyncReadableWritable RW>
    class ReceiveInformationResponseTask {
        struct WaitingForInformatinResponse {
            // 説明書には6msとあるが、IRレスポンス送信終了まで待機+余裕を見て20msにしている
            static constexpr auto TIMEOUT = util::Duration::from_millis(20);

            nb::Delay timeout_;

            explicit WaitingForInformatinResponse(util::Time &time) : timeout_{time, TIMEOUT} {}
        };

        struct ReceivingInformationResponse {
            nb::LockGuard<etl::reference_wrapper<RW>> readable;
            UhfFixedResponseBodyDeserializer<2> response;

            explicit ReceivingInformationResponse(nb::LockGuard<etl::reference_wrapper<RW>> &&r)
                : readable{etl::move(r)} {}

            inline nb::Poll<nb::de::DeserializeResult> execute() {
                return response.deserialize(readable->get());
            }
        };

        etl::variant<WaitingForInformatinResponse, ReceivingInformationResponse> state_;

      public:
        explicit ReceiveInformationResponseTask(util::Time &time)
            : state_{WaitingForInformatinResponse{time}} {}

        nb::Poll<InformationResponseResult>
        execute(nb::Lock<etl::reference_wrapper<RW>> &rw, util::Time &time) {
            if (etl::holds_alternative<WaitingForInformatinResponse>(state_)) {
                auto &state = etl::get<WaitingForInformatinResponse>(state_);
                POLL_UNWRAP_OR_RETURN(state.timeout_.poll(time));
                return nb::ready(InformationResponseResult::Ok);
            }

            if (etl::holds_alternative<ReceivingInformationResponse>(state_)) {
                auto &state = etl::get<ReceivingInformationResponse>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute());
                return nb::ready(InformationResponseResult::Error);
            }

            return nb::pending;
        }

        UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            if (etl::holds_alternative<WaitingForInformatinResponse>(state_) &&
                res.type == UhfResponseType::IR) {
                state_.template emplace<ReceivingInformationResponse>(etl::move(res.body));
                return UhfHandleResponseResult::Handle;
            }

            return UhfHandleResponseResult::Invalid;
        }
    };

    template <nb::AsyncReadableWritable RW>
    class SendDataTask {
        struct Initial {};

        using SendData = FixedTask<RW, AsyncSendDataCommandSerializer, UhfResponseType::DT, 2>;
        static constexpr auto MAX_RETRY_COUNT = 10;

        UhfFrame frame_;
        uint8_t remaining_retry_count_{MAX_RETRY_COUNT};
        etl::variant<Initial, CarrierSenseTask<RW>, SendData, ReceiveInformationResponseTask<RW>>
            task_;

      public:
        explicit SendDataTask(UhfFrame &&frame, util::Time &time, util::Rand &rand)
            : frame_{etl::move(frame)},
              task_{CarrierSenseTask<RW>{time, rand}} {}

        nb::Poll<void>
        execute(nb::Lock<etl::reference_wrapper<RW>> &rw, util::Time &time, util::Rand &rand) {
            if (etl::holds_alternative<Initial>(task_)) {
                task_.template emplace<CarrierSenseTask<RW>>(time, rand);
            }

            if (etl::holds_alternative<CarrierSenseTask<RW>>(task_)) {
                auto &task = etl::get<CarrierSenseTask<RW>>(task_);
                auto result = POLL_UNWRAP_OR_RETURN(task.execute(rw, time, rand));
                task_.template emplace<SendData>(frame_.clone());
                if (result == CarrierSenseResult::Error) {
                    return nb::ready();
                }
            }

            if (etl::holds_alternative<SendData>(task_)) {
                auto &state = etl::get<SendData>(task_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(rw));
                task_.template emplace<ReceiveInformationResponseTask<RW>>(time);
                if (result == FixedTaskResult::Error) {
                    return nb::ready();
                }
            }

            if (etl::holds_alternative<ReceiveInformationResponseTask<RW>>(task_)) {
                auto &state = etl::get<ReceiveInformationResponseTask<RW>>(task_);
                auto result = POLL_UNWRAP_OR_RETURN(state.execute(rw, time));
                if (result == InformationResponseResult::Ok || remaining_retry_count_ == 0) {
                    return nb::ready();
                }

                remaining_retry_count_--;
                task_.template emplace<CarrierSenseTask<RW>>(time, rand);
            }

            return nb::pending;
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            return etl::visit(
                tl::Visitor{
                    [&](Initial &) { return UhfHandleResponseResult::Invalid; },
                    [&](auto &state) { return state.handle_response(etl::move(res)); }
                },
                task_
            );
        }

        inline TaskInterruptionResult interrupt() {
            task_.template emplace<Initial>();
            return TaskInterruptionResult::Interrupted;
        }
    };
} // namespace media::uhf
