#pragma once

#include <etl/functional.h>
#include <etl/utility.h>
#include <nb/poll.h>
#include <util/tuple.h>

namespace nb::stream {
    template <typename T>
    class StreamWriter;

    class StreamReader {
      public:
        using Item = uint8_t;

        virtual nb::Poll<Item> read() = 0;
        virtual nb::Poll<void> wait_until_empty() = 0;

      private:
        nb::Poll<void> fill_single(StreamWriter<Item> &writer);

      public:
        /**
         * `fixed_ws`のデータ全てを`this`に吐き出す．
         * @param fixed_ws `this`にデータを吐き出す，書き込み可能な固定長ストリーム
         */
        template <typename... FixedWs>
        nb::Poll<void> fill_all(FixedWs &&...fixed_ws) {
            (fill_single(etl::forward<FixedWs>(fixed_ws)).is_ready() && ...);
            return nb::ready;
        }
    };

    template <typename T>
    class StreamWriter {
        friend class StreamReader;

      public:
        using Item = uint8_t;

        virtual nb::Poll<T> poll() = 0;

      protected:
        virtual nb::Poll<void> wait_until_writable() = 0;
        virtual void write(Item) = 0;

      private:
        inline nb::Poll<void> drain_single(StreamReader &reader) {
            while (wait_until_writable().is_ready()) {
                write(POLL_UNWRAP_OR_RETURN(reader.read()));

                if (reader.wait_until_empty().is_ready()) {
                    return nb::ready;
                }
            }
            return nb::pending;
        }

      public:
        /**
         * `fixed_rs`のデータ全てを`this`に吸い込む．
         * @param fixed_rs `this`にデータを吸い込まれる，読み込み可能な固定長ストリーム
         */
        template <typename... FixedRs>
        nb::Poll<T> drain_all(FixedRs &&...fixed_rs) {
            (drain_single(etl::forward<FixedRs>(fixed_rs)).is_ready() && ...);
            return poll();
        }
    };
} // namespace nb::stream
