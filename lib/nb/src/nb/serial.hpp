#pragma once

#include <etl/optional.h>
#include <etl/type_traits.h>

namespace nb {

/**
 * ノンブロッキング版のシリアル型
 *
 * # Example
 *
 * ```
 * #include <Arduino>
 * #include <nb/serial.hpp>
 *
 * nb::Serial<HardwareSerial> serial(Serial);
 *
 * void setup() {
 *     Serial.begin(115200); // Arduino側のシリアル通信を開始
 *     serial = nb::Serial(Serial);
 * }
 * ```
 */
template <typename T>
class Serial {
    T serial_;

  public:
    Serial(const Serial &) = delete;
    Serial &operator=(const Serial) = delete;

    Serial(Serial &&) = default;
    Serial &operator=(Serial &&) = default;

    Serial(T &serial) : serial_(serial) {}

    /**
     * シリアルを利用可能かどうかを返す．
     *
     * # Example
     *
     * ```
     * nb::Serial<HardwareSerial> serial(Serial);
     * serial.is_ready();
     * ```
     *
     */
    bool is_ready() {
        return static_cast<bool>(serial_);
    }

    /**
     * シリアルから1バイト読み取る．
     * 値がない場合は，空の値を返す．
     *
     * # Example
     *
     * ```
     * nb::Serial<HardwareSerial> serial(Serial);
     * etl::optional<uint8_t> data = serial.read();
     * ```
     */
    etl::optional<uint8_t> read() {
        int data = serial_.read();
        if (data == -1) {
            return etl::nullopt;
        } else {
            return static_cast<uint8_t>(data);
        }
    }

    /**
     * シリアルに1バイトを書き込む．
     * 書き込身に成功した場合は`true`を返し，
     * バッファが満杯で書き込む余裕がない場合は`false`を返す
     *
     * # Example
     *
     * ```
     *
     * nb::Serial<HardwareSerial> serial(Serial);
     * serial.write(42);
     * ```
     *
     */
    bool write(uint8_t data) {
        if (serial_.availableForWrite() == 0) {
            return false;
        } else {
            serial_.write(data);
            return true;
        }
    }

    /**
     * まだ読み取っていない値のバイト数を返す．
     *
     * # Example
     *
     * ```
     * nb::Serial<HardwareSerial> serial(Serial);
     * serial.readable_bytes();
     * ```
     */
    uint8_t readable_bytes() {
        int available_bytes = serial_.available();
        return static_cast<uint8_t>(available_bytes);
    }

    /**
     * シリアルに書き込めるバイト数を返す．
     *
     * # Example
     *
     * ```
     * nb::Serial<HardwareSerial> serial(Serial);
     * serial.writable_bytes();
     * ```
     */
    uint8_t writable_bytes() {
        int available_bytes = serial_.availableForWrite();
        return static_cast<uint8_t>(available_bytes);
    }
};

template <typename T>
using HasSerialMethod =
    etl::conjunction<etl::is_same<decltype(etl::declval<T>().is_ready()), bool>,
                     etl::is_same<decltype(etl::declval<T>().read()), etl::optional<uint8_t>>,
                     etl::is_same<decltype(etl::declval<T>().write(etl::declval<uint8_t>())), void>,
                     etl::is_same<decltype(etl::declval<T>().readable_bytes()), uint8_t>,
                     etl::is_same<decltype(etl::declval<T>().writable_bytes()), uint8_t>>;

template <typename T, typename = void>
struct IsSerial : etl::false_type {};

/**
 * 型`T`がnb::Serialのインターフェースと満たしていることを検証する
 */
template <typename T>
struct IsSerial<T, etl::void_t<HasSerialMethod<T>>> : HasSerialMethod<T> {};

} // namespace nb
