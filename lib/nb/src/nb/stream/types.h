#pragma once

#include <etl/span.h>
#include <nb/poll.h>

namespace nb::stream {
    /**
     * バイト列を読み取ることが出来るストリーム．
     */
    class ReadableStream {
      public:
        /**
         * 呼び出されたときに確実に読み取れるバイト数を返す．
         * @return 読み取れるバイト数
         */
        virtual uint8_t readable_count() const = 0;

        /**
         * 1バイト読み取る．
         * @pre `readable_count() > 0`
         * @return 読み取ったバイト
         */
        virtual uint8_t read() = 0;

        /**
         * `buffer`に読み取ったバイト列を書き込む．
         * @pre `readable_count() >= buffer.size()`
         * @param buffer 書き込み先
         */
        virtual void read(etl::span<uint8_t> buffer) {
            for (auto &byte : buffer) {
                byte = read();
            }
        }
    };

    /**
     * バイト列を書き込むことが出来るストリーム．
     *
     * `write`関数はまだ書き込めるかどうかを返すが，これは書き込まれた内容によって，
     * 動的に書き込めるバイト数が変わる場合に対応するためである．
     */
    class WritableStream {
      public:
        /**
         * 呼び出されたときに確実に書き込めるバイト数を返す．
         * @return 書き込めるバイト数
         */
        virtual uint8_t writable_count() const = 0;

        /**
         * 1バイト書き込む．
         * @pre `writable_count() > 0`
         * @param byte 書き込むバイト
         * @return まだ書き込める場合は`true`，そうでない場合は`false`
         */
        virtual bool write(uint8_t) = 0;

        /**
         * `buffer`のバイト列を書き込む．
         * @pre `writable_count() >= buffer.size()`
         * @param buffer 書き込むバイト列
         * @return まだ書き込める場合は`true`，そうでない場合は`false`
         */
        virtual bool write(etl::span<uint8_t> buffer) {
            for (uint8_t byte : buffer) {
                write(byte);
            }
            return writable_count() > 0;
        }
    };

    /**
     * バイト列を`WritableStream`に吐き出すことが出来る型．
     *
     * `write_to`が右辺値参照を受け取るオーバーロードがないが，
     * これは繰り返し`write_to(HogeWritableStream{})`のように間違った呼ばれ方をすると，
     * 正しく書き込んだデータを蓄積できないためである．
     */
    class ReadableBuffer {
      public:
        /**
         * `this`のデータを吐き出せるだけ全て`destination`に吐き出す．
         * @param destination `this`からデータを吐き出される，書き込み可能なストリーム
         * @return `this`が空になった場合は`nb::ready()`，そうでない場合は`nb::pending`
         */
        virtual nb::Poll<void> write_to(WritableStream &destination) = 0;
    };

    /**
     * バイト列を`ReadableStream`に吸い込ませることが出来る型．
     *
     * `read_from`が右辺値参照を受け取るオーバーロードがない理由は`ReadableBuffer`を参照．
     */
    class WritableBuffer {
      public:
        /**
         * `source`からデータを吸い込めるだけ全て`this`に吸い込む．
         * @param source `this`にデータを吸い込まれる，読み込み可能なストリーム
         * @return 読み込みが完了した場合は`nb::ready()`，そうでない場合は`nb::pending`
         */
        virtual nb::Poll<void> read_from(ReadableStream &source) = 0;
    };

    /**
     * `reader`から読み取ったバイト列を`wb`に書き込む．
     *
     * 全ての`wb`を埋めることが出来た場合は`nb::ready()`を返し，そうでない場合は`nb::pending`を返す．
     *
     * @note `wb`は`WritableBuffer`を継承する必要がある．
     */
    template <typename... WBuffers>
    nb::Poll<void> write_all(ReadableStream &reader, WBuffers &&...wb) {
        const bool is_ready = (wb.read_from(reader).is_ready() && ...);
        return is_ready ? nb::ready() : nb::pending;
    }

    /**
     * `rb`から読み取ったバイト列を`writer`に書き込む．
     *
     * 全ての`rb`を空にできた場合は`nb::ready()`を返し，そうでない場合は`nb::pending`を返す．
     *
     * @note `rb`は`ReadableBuffer`を継承する必要がある．
     */
    template <typename... RBuffers>
    nb::Poll<void> read_all(WritableStream &writer, RBuffers &&...rb) {
        const bool is_ready = (rb.write_to(writer).is_ready() && ...);
        return is_ready ? nb::ready() : nb::pending;
    }
} // namespace nb::stream
