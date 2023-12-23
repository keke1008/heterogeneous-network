#pragma once

#include "../common.h"
#include "../response.h"
#include "./carrier_sence.h"
#include "./fixed.h"
#include <nb/serde.h>
#include <nb/time.h>
#include <net/frame.h>

namespace net::link::uhf {
    class AsyncSendDataCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer prefix_{"@DT"};
        nb::ser::Hex<uint8_t> length_;
        frame::AsyncProtocolNumberSerializer protocol_;
        frame::AsyncFrameBufferReaderSerializer payload_;
        nb::ser::AsyncStaticSpanSerializer route_prefix_{"/R"};
        AsyncModemIdSerializer destination_;
        nb::ser::AsyncStaticSpanSerializer suffix_{"\r\n"};

      public:
        explicit AsyncSendDataCommandSerializer(UhfFrame &&frame)
            : length_{static_cast<uint8_t>(frame.reader.buffer_length() + frame::PROTOCOL_SIZE)},
              protocol_{frame.protocol_number},
              payload_{etl::move(frame.reader)},
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

        nb::Poll<void> execute(nb::Lock<etl::reference_wrapper<RW>> &rw, util::Time &time) {
            if (etl::holds_alternative<WaitingForInformatinResponse>(state_)) {
                return etl::get<WaitingForInformatinResponse>(state_).timeout_.poll(time);
            }

            if (etl::holds_alternative<ReceivingInformationResponse>(state_)) {
                auto &state = etl::get<ReceivingInformationResponse>(state_);
                POLL_UNWRAP_OR_RETURN(state.execute());
                return nb::ready();
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
        using SendData = FixedTask<RW, AsyncSendDataCommandSerializer, UhfResponseType::DT, 2>;

        etl::optional<CarrierSenseTask<RW>> carrier_sense_{};
        tl::Optional<SendData> send_data_;
        ReceiveInformationResponseTask<RW> receive_information_response_;

      public:
        explicit SendDataTask(UhfFrame &&frame, util::Time &time)
            : send_data_{SendData{etl::move(frame)}},
              receive_information_response_{time} {}

        nb::Poll<void>
        execute(nb::Lock<etl::reference_wrapper<RW>> &rw, util::Time &time, util::Rand &rand) {
            if (carrier_sense_) {
                auto result = POLL_UNWRAP_OR_RETURN(carrier_sense_->execute(rw, time, rand));
                carrier_sense_.reset();
                if (result == CarrierSenseResult::Error) {
                    return nb::ready();
                }
            }
            if (send_data_) {
                auto result = POLL_UNWRAP_OR_RETURN(send_data_->execute(rw));
                send_data_.reset();
                if (result == FixedTaskResult::Error) {
                    return nb::ready();
                }
            }
            return receive_information_response_.execute(rw, time);
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            if (carrier_sense_) {
                return carrier_sense_->handle_response(etl::move(res));
            } else if (send_data_) {
                return send_data_->handle_response(etl::move(res));
            } else {
                return receive_information_response_.handle_response(etl::move(res));
            }
        }
    };
} // namespace net::link::uhf
