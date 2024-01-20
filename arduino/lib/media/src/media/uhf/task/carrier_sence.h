#pragma once

#include "../response.h"
#include "./fixed.h"
#include <nb/serde.h>
#include <nb/time.h>
#include <net/frame.h>

namespace media::uhf {
    enum class CarrierSenseResult : uint8_t {
        Ok,
        Error,
    };

    template <nb::AsyncReadableWritable RW>
    class CarrierSenseTask {
        // 300byte(256byteデータ+適当な制御用byte) / 4.8kbps = 62.5ms
        // 62.5msの前後でランダムにバックオフする
        static constexpr util::TimeInt BACKOFF_RANGE_MS = 100;
        static constexpr util::TimeInt BACKOFF_OFFSET_MS = 50;

        struct Backoff {
            nb::Delay delay_;

          private:
            explicit Backoff(util::Time &time, util::Rand &rand, util::TimeInt offset_ms)
                : delay_{([&]() -> nb::Delay {
                      auto backoff = rand.gen_uint8_t(offset_ms, BACKOFF_RANGE_MS + offset_ms);
                      return nb::Delay{time, util::Duration::from_millis(backoff)};
                  })()} {}

          public:
            static Backoff WithoutOffset(util::Time &time, util::Rand &rand) {
                return Backoff{time, rand, 0};
            }

            static Backoff WithOffset(util::Time &time, util::Rand &rand) {
                return Backoff{time, rand, BACKOFF_OFFSET_MS};
            }
        };

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
            state_.template emplace<Backoff>(Backoff::WithOffset(time, rand));
            return nb::pending;
        }

      public:
        constexpr CarrierSenseTask(util::Time &time, util::Rand &rand)
            // ブロードキャストされたフレームを中継する際，
            // 複数のノードの送信タイミングがぴったりになっている場合がよくあり，
            // CS=ENを得た場合でも，すぐに送信を開始すると衝突する可能性がある．
            // そのため，最初にランダムなバックオフを行う．
            : state_{Backoff::WithoutOffset(time, rand)} {}

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
} // namespace media::uhf
