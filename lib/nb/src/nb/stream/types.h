#pragma once

#include <debug_assert.h>
#include <etl/span.h>
#include <etl/string_view.h>
#include <nb/poll.h>

namespace nb::stream {
    /**
     * バイト列を読み取ることが出来るストリーム．
     *
     * 保持しているバイト列をすべて読み込む必要がない使用方法がある，
     * もしくは無限に読み込める場合はこのクラスを継承すべき．
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
     * 保持しているバイト列をすべて書き込む必要がない使用方法がある，
     * もしくは無限に書き込める場合はこのクラスを継承すべき．
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
        virtual bool write(etl::span<const uint8_t> buffer) {
            for (uint8_t byte : buffer) {
                write(byte);
            }
            return writable_count() > 0;
        }

        /**
         * `view`のバイト列を書き込む．デバッグ用
         * @pre `writable_count() >= view.size()`
         * @param view 書き込むバイト列
         * @return まだ書き込める場合は`true`，そうでない場合は`false`
         */
        bool write_str(const etl::string_view view) {
            for (uint8_t byte : view) {
                write(byte);
            }
            return writable_count() > 0;
        }
    };

    class ReadableWritableStream : public ReadableStream, public WritableStream {};

    /**
     * バイト列を`WritableStream`に吐き出すことが出来る型．
     *
     * 保持しているバイト列をすべて吐き出すような使用方法がある場合は，このクラスを継承すべき．
     *
     * `read_all_into`が右辺値参照を受け取るオーバーロードがないが，
     * これは繰り返し`read_all_into(HogeWritableStream{})`のように間違った呼ばれ方をすると，
     * 正しく書き込んだデータを蓄積できないためである．
     */
    class ReadableBuffer {
      public:
        /**
         * `this`のデータを吐き出せるだけ全て`destination`に吐き出す．
         * @param destination `this`からデータを吐き出される，書き込み可能なストリーム
         * @return `this`が空になった場合は`nb::ready()`，そうでない場合は`nb::pending`
         */
        virtual nb::Poll<void> read_all_into(WritableStream &destination) = 0;
    };

    /**
     * バイト列を`ReadableStream`に吸い込ませることが出来る型．
     *
     * 保持しているバイト列をすべて吸い込ませるような使用方法がある場合は，このクラスを継承すべき．
     *
     * `write_all_from`が右辺値参照を受け取るオーバーロードがない理由は`ReadableBuffer`を参照．
     */
    class WritableBuffer {
      public:
        /**
         * `source`からデータを吸い込めるだけ全て`this`に吸い込む．
         * @param source `this`にデータを吸い込まれる，読み込み可能なストリーム
         * @return `this`が満杯になった場合は`nb::ready()`，そうでない場合は`nb::pending`
         */
        virtual nb::Poll<void> write_all_from(ReadableStream &source) = 0;
    };

    class ReadableWritableBuffer : public ReadableBuffer, public WritableBuffer {};

    /**
     * `rb`から読み取ったバイト列を`writer`に書き込む．
     *
     * 全ての`rb`を空にできた場合は`nb::ready()`を返し，そうでない場合は`nb::pending`を返す．
     *
     * @note `rb`は`ReadableBuffer`を継承する必要がある．
     */
    template <typename... RBuffers>
    nb::Poll<void> read_all_into(WritableStream &writer, RBuffers &&...rb) {
        const bool is_ready = (rb.read_all_into(writer).is_ready() && ...);
        return is_ready ? nb::ready() : nb::pending;
    }

    /**
     * `reader`から読み取ったバイト列を`wb`に書き込む．
     *
     * 全ての`wb`を埋めることが出来た場合は`nb::ready()`を返し，そうでない場合は`nb::pending`を返す．
     *
     * @note `wb`は`WritableBuffer`を継承する必要がある．
     */
    template <typename... WBuffers>
    nb::Poll<void> write_all_from(ReadableStream &reader, WBuffers &&...wb) {
        const bool is_ready = (wb.write_all_from(reader).is_ready() && ...);
        return is_ready ? nb::ready() : nb::pending;
    }
} // namespace nb::stream
