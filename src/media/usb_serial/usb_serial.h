#pragma once

#include "./receiver.h"
#include "./sender.h"
#include "media/packet.h"
#include <mock/serial.h>
#include <nb/serial.h>
#include <nb/stream.h>

namespace media {
    template <typename T>
    class UsbSerial {
        nb::serial::Serial<T> serial_;
        usb_serial::UsbSerialReceiver receiver_;
        usb_serial::UsbSerialSender sender_;

      public:
        using WritableStreamItem = UsbSerialPacket;
        using ReadableStreamItem = UsbSerialPacket;

        UsbSerial() = default;
        UsbSerial(const UsbSerial &) = default;
        UsbSerial(UsbSerial &&) = default;
        UsbSerial &operator=(const UsbSerial &) = default;
        UsbSerial &operator=(UsbSerial &&) = default;

        UsbSerial(nb::serial::Serial<T> &&serial) : serial_{etl::move(serial)} {}

        void execute() {
            nb::stream::pipe(serial_, receiver_);
            nb::stream::pipe(sender_, serial_);
        }

        size_t writable_count() const {
            return sender_.writable_count();
        }

        size_t readable_count() const {
            return receiver_.readable_count();
        }

        bool write(UsbSerialPacket &&packet) {
            return sender_.write(etl::move(packet));
        }

        etl::optional<UsbSerialPacket> read() {
            return receiver_.read();
        }

        bool is_closed() const {
            return false;
        }
    };

    static_assert(nb::stream::is_stream_writer_v<UsbSerial<mock::MockSerial>, UsbSerialPacket>);
    static_assert(nb::stream::is_stream_reader_v<UsbSerial<mock::MockSerial>, UsbSerialPacket>);
} // namespace media
