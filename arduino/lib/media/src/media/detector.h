#pragma once

#include <logger.h>
#include <memory/lifetime.h>
#include <nb/poll.h>
#include <nb/serde.h>
#include <nb/time.h>
#include <net.h>
#include <util/span.h>
#include <util/time.h>

namespace media {
    template <nb::AsyncReadableWritable RW>
    class MediaDetector {
        memory::Static<RW> &stream_;

        // 試しに送ってみるコマンド．
        // UHFモデムのコマンドは@SN\r\nを送るとシリアル番号が返ってくる．
        // WifiモデムのコマンドはERRORが返ってくる．
        // どちらも返答がない場合はシリアルポートと判断する．
        nb::ser::AsyncStaticSpanSerializer command_{"@SN\r\n"};

        nb::de::AsyncMaxLengthSingleLineBytesDeserializer<15> response_;
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

        nb::Poll<net::link::MediaType> poll(util::Time &time) {
            POLL_UNWRAP_OR_RETURN(begin_transmission_.poll(time));
            POLL_UNWRAP_OR_RETURN(command_.serialize(*stream_));

            // 何も返答がない場合
            if (time.now() > stop_receiving_) {
                LOG_INFO(FLASH_STRING("Detected: Serial"));
                return nb::ready(net::link::MediaType::Serial);
            }

            while (stream_->poll_readable(1).is_ready()) {
                auto result = POLL_UNWRAP_OR_RETURN(response_.deserialize(*stream_));
                if (result != nb::DeserializeResult::Ok) {
                    response_.reset();
                    continue;
                }

                auto span = response_.result();

                // UHFモデムのシリアル番号のレスポンスの場合
                if (util::as_str(span.first<4>()) == "*SN=") {
                    LOG_INFO(FLASH_STRING("Detected: UHF"));
                    return nb::ready(net::link::MediaType::UHF);
                }

                // Wifiシールドのエラーレスポンスの場合
                if (util::as_str(span) == "ERROR\r\n") {
                    LOG_INFO(FLASH_STRING("Detected: Wifi"));
                    return nb::ready(net::link::MediaType::Wifi);
                }

                response_.reset();
            }

            return nb::pending;
        }
    };
} // namespace media
