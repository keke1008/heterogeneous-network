#pragma once

#include "./receiver.h"
#include "./sender.h"
#include "media/common.h"
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
        using StreamWriterItem = UsbSerialPacket;
        using StreamReaderItem = UsbSerialPacket;

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

        bool is_writable() const {
            return sender_.is_writable();
        }

        size_t writable_count() const {
            return sender_.writable_count();
        }

        bool write(UsbSerialPacket &&packet) {
            return sender_.write(etl::move(packet));
        }

        bool is_readable() const {
            return receiver_.is_readable();
        }

        size_t readable_count() const {
            return receiver_.readable_count();
        }

        etl::optional<UsbSerialPacket> read() {
            return receiver_.read();
        }

        bool is_closed() const {
            return false;
        }
    };
} // namespace media
