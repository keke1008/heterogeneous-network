#pragma once

#include "../response.h"
#include "./fixed.h"
#include <nb/serde.h>
#include <nb/time.h>
#include <net/frame.h>

namespace net::link::uhf {
    enum class CarrierSenseResult : uint8_t {
        Ok,
        Error,
    };

    template <nb::AsyncReadableWritable RW>
    class CarrierSenseTask {

        struct Backoff {
            nb::Delay delay_;
        };

        // 300byte(256byteデータ+適当な制御用byte) / 4.8kbps = 62.5ms
        // 62.5msの前後でランダムにバックオフする
        static inline constexpr util::TimeInt BACKOFF_MIN_MS = 50;
        static inline constexpr util::TimeInt BACKOFF_MAX_MS = 100;
        static inline constexpr uint8_t MAX_RETRY_COUNT = 15;

        static constexpr auto CS_COMMAND = "@CS\r\n";
        using SendCommand =
            FixedTask<RW, nb::ser::AsyncStaticSpanSerializer, UhfResponseType::CS, 2>;

        uint8_t remaining_retry_count_{MAX_RETRY_COUNT};
        etl::variant<SendCommand, Backoff> state_{SendCommand{CS_COMMAND}};

        nb::Poll<CarrierSenseResult> backoff(util::Time &time, util::Rand &rand) {
            if (remaining_retry_count_ == 0) {
                return CarrierSenseResult::Error;
            }

            remaining_retry_count_--;
            auto backoff = rand.gen_uint8_t(BACKOFF_MIN_MS, BACKOFF_MAX_MS);
            state_.template emplace<Backoff>(nb::Delay{time, util::Duration::from_millis(backoff)});
            return nb::pending;
        }

      public:
        nb::Poll<CarrierSenseResult>
        execute(nb::Lock<etl::reference_wrapper<RW>> &rw, util::Time &time, util::Rand &rand) {
            if (etl::holds_alternative<Backoff>(state_)) {
                POLL_UNWRAP_OR_RETURN(etl::get<Backoff>(state_).delay_.poll(time));
                state_.template emplace<SendCommand>(CS_COMMAND);
            }

            if (etl::holds_alternative<SendCommand>(state_)) {
                auto &state = etl::get<SendCommand>(state_);
                if (POLL_UNWRAP_OR_RETURN(state.execute(rw)) != FixedTaskResult::Ok) {
                    return backoff(time, rand);
                }

                if (state.result() != "EN") {
                    return backoff(time, rand);
                }

                return CarrierSenseResult::Ok;
            }

            return nb::pending;
        }

        inline UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            if (etl::holds_alternative<SendCommand>(state_)) {
                return etl::get<SendCommand>(state_).handle_response(etl::move(res));
            } else {
                return UhfHandleResponseResult::Invalid;
            }
        }
    };
} // namespace net::link::uhf
