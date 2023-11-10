#pragma once

#include "./media.h"
#include <logger.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <nb/time.h>
#include <util/span.h>
#include <util/time.h>

namespace net::link {
    class MediaDetector {
        nb::stream::ReadableWritableStream &stream_;
        nb::stream::FixedReadableBuffer<4> command_{"AT\r\n"};
        nb::stream::MaxLengthSingleLineWrtableBuffer<8> buffer_;
        nb::Delay begin_transmission_;
        util::Instant stop_receiving_;

      public:
        MediaDetector(nb::stream::ReadableWritableStream &serial, util::Time &time)
            : stream_{serial},

              // 電源投入から100msはUHFモデムのコマンド発行禁止期間
              // 50msaは適当に決めた余裕
              begin_transmission_{time, util::Duration::from_millis(150)},

              // 250 - 150 = 100msは適当に決めたレスポンス待ち時間
              stop_receiving_{time.now() + util::Duration::from_millis(250)} {}

        nb::stream::ReadableWritableStream &stream() {
            return stream_;
        }

        nb::Poll<MediaType> poll(util::Time &time) {
            POLL_UNWRAP_OR_RETURN(begin_transmission_.poll(time));
            POLL_UNWRAP_OR_RETURN(command_.read_all_into(stream_));

            // 何も返答がない場合
            if (time.now() > stop_receiving_) {
                LOG_INFO("Detected: Serial");
                return nb::ready(MediaType::Serial);
            }

            while (stream_.readable_count() > 0) {
                POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream_));

                // ATコマンドのレスポンスの場合
                auto opt_span = buffer_.written_bytes();
                if (!opt_span.has_value()) {
                    buffer_.reset();
                    continue;
                }

                if (util::as_str(*opt_span) == "OK\r\n") {
                    LOG_INFO("Detected: Wifi");
                    return nb::ready(MediaType::Wifi);
                }

                // UHFモデムのエラーレスポンスの場合
                if (util::as_str(opt_span->first<4>()) == "*ER=") {
                    LOG_INFO("Detected: UHF");
                    return nb::ready(MediaType::UHF);
                }

                buffer_.reset();
            }

            return nb::pending;
        }
    };
} // namespace net::link
