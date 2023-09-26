#pragma once

#include "./media.h"
#include <nb/poll.h>
#include <nb/stream.h>
#include <nb/time.h>
#include <util/span.h>
#include <util/time.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

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

            while (stream_.writable_count() > 0) {
                POLL_UNWRAP_OR_RETURN(buffer_.write_all_from(stream_));

                // ATコマンドのレスポンスの場合
                auto span = buffer_.written_bytes();
                if (etl::equal(span, etl::span("OK\r\n"_u8array))) {
                    return nb::ready(MediaType::Wifi);
                }

                // UHFモデムのエラーレスポンスの場合
                if (etl::equal(span.first<4>(), etl::span("*ER="_u8array))) {
                    return nb::ready(MediaType::UHF);
                }

                // 何も返答がない場合
                if (time.now() > stop_receiving_) {
                    return nb::ready(MediaType::Serial);
                }

                buffer_.reset();
            }

            return nb::pending;
        }
    };
} // namespace net::link
