#pragma once

#include <HardwareSerial.h>
#include <bytes/serde.h>
#include <etl/array.h>
#include <etl/optional.h>
#include <etl/utility.h>
#include <nb/lock.h>
#include <nb/serial.h>
#include <nb/stream.h>

namespace media::usb_serial {
    using nb::serial::Serial, nb::stream::FixedByteReader, nb::lock::Lock, nb::lock::Guard;

    template <typename R>
    class PacketSender {

        Guard<Serial<HardwareSerial>> serial_;
        FixedByteReader<2> size_reader_;
        R data_reader_;

      public:
        PacketSender(
            Guard<Serial<HardwareSerial>> &&serial,
            FixedByteReader<2> size_reader,
            R &&data_reader
        )
            : serial_{etl::move(serial)},
              size_reader_{size_reader},
              data_reader_{etl::forward<R>(data_reader)} {}

        bool execute() {
            return nb::stream::pipe_readers(*serial_, size_reader_, data_reader_);
        }
    };

    class UsbSerial {
        Lock<Serial<HardwareSerial>> serial_;

      public:
        UsbSerial(Serial<HardwareSerial> &&serial) : serial_{etl::move(serial)} {}

        template <typename R>
        etl::optional<PacketSender<R>> send(uint16_t size, R &&data_reader) {
            auto guard = serial_.lock();
            if (guard.has_value()) {
                return PacketSender{
                    etl::move(*guard),
                    bytes::serialize(size),
                    etl::forward<R>(data_reader),
                };
            } else {
                return etl::nullopt;
            }
        }
    };
} // namespace media::usb_serial
