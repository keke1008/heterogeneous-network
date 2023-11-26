#pragma once

#include "./media.h"
#include <logger.h>
#include <nb/poll.h>
#include <nb/time.h>
#include <util/span.h>
#include <util/time.h>

namespace net::link {
    template <nb::AsyncReadableWritable RW>
    class MediaDetector {
        memory::Static<RW> &stream_;
        nb::ser::AsyncStaticSpanSerializer command_{"AT\r\n"};
        nb::de::AsyncMaxLengthSingleLineBytesDeserializer<8> response_;
        nb::Delay begin_transmission_;
        util::Instant stop_receiving_;

      public:
        MediaDetector(memory::Static<RW> &serial, util::Time &time)
            : stream_{serial},

              // 電源投入から100msはUHFモデムのコマンド発行禁止期間
              // 50msaは適当に決めた余裕
              begin_transmission_{time, util::Duration::from_millis(150)},

              // 250 - 150 = 100msは適当に決めたレスポンス待ち時間
              stop_receiving_{time.now() + util::Duration::from_millis(250)} {}

        memory::Static<RW> &stream() {
            return stream_;
        }

        nb::Poll<MediaType> poll(util::Time &time) {
            POLL_UNWRAP_OR_RETURN(begin_transmission_.poll(time));
            POLL_UNWRAP_OR_RETURN(command_.serialize(*stream_));

            // 何も返答がない場合
            if (time.now() > stop_receiving_) {
                LOG_INFO("Detected: Serial");
                return nb::ready(MediaType::Serial);
            }

            while (stream_->poll_readable(1).is_ready()) {
                auto result = POLL_UNWRAP_OR_RETURN(response_.deserialize(*stream_));
                if (result != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                auto span = response_.result();

                // ATコマンドのレスポンスの場合
                auto &str = util::as_str(span);
                if (str == "OK\r\n" || str == "ERROR\r\n") { // 何故かERRORが返ってくることがある
                    LOG_INFO("Detected: Wifi");
                    return nb::ready(MediaType::Wifi);
                }

                // UHFモデムのエラーレスポンスの場合
                if (util::as_str(span.first<4>()) == "*ER=") {
                    LOG_INFO("Detected: UHF");
                    return nb::ready(MediaType::UHF);
                }

                response_.reset();
            }

            return nb::pending;
        }
    };
} // namespace net::link
