#pragma once

#include <etl/functional.h>
#include <etl/utility.h>
#include <nb/poll.h>
#include <util/tuple.h>

namespace nb::stream {
    class FiniteStreamWriter;

    class FiniteStreamReader;

    class StreamReader {
      public:
        using Item = uint8_t;

        virtual nb::Poll<Item> read() = 0;

      private:
        nb::Poll<void> fill_single(FiniteStreamWriter &writer);

      public:
        /**
         * `this`のデータを全て`ws`に吐き出す．
         * @param ws `this`からデータを吐き出される，書き込み可能な有限ストリーム
         */
        template <typename... FiniteWriters>
        nb::Poll<void> fill_all(FiniteWriters &&...ws) {
            (fill_single(etl::forward<FiniteWriters>(ws)).is_ready() && ...);
            return nb::ready();
        }
    };

    class FiniteStreamReader : public StreamReader {
      public:
        virtual nb::Poll<void> wait_until_empty() = 0;
    };

    class StreamWriter {
        friend class StreamReader;

      public:
        using Item = uint8_t;

      protected:
        virtual nb::Poll<void> wait_until_writable() = 0;
        virtual void write(Item) = 0;

      private:
        inline nb::Poll<void> drain_single(FiniteStreamReader &reader) {
            while (wait_until_writable().is_ready()) {
                write(POLL_UNWRAP_OR_RETURN(reader.read()));

                if (reader.wait_until_empty().is_ready()) {
                    return nb::ready();
                }
            }
            return nb::pending;
        }

        inline nb::Poll<void> drain_single(FiniteStreamReader &&reader) {
            return drain_single(reader);
        }

      public:
        /**
         * `rs`のデータ全てを`this`に吸い込む．
         * @param rs `this`にデータを吸い込まれる，読み込み可能な有限ストリーム
         */
        template <typename... FiniteReader>
        nb::Poll<void> drain_all(FiniteReader &&...rs) {
            bool is_ready = (drain_single(etl::forward<FiniteReader>(rs)).is_ready() && ...);
            return is_ready ? nb::ready() : nb::pending;
        }
    };

    class FiniteStreamWriter : public StreamWriter {};
} // namespace nb::stream
