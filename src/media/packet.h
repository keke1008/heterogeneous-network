#pragma once

#include <bytes/serde.h>
#include <etl/variant.h>
#include <nb/stream.h>

namespace media {
    template <typename T>
    class MediaAddress {
        T value_;

      public:
        MediaAddress() = delete;
        MediaAddress(const MediaAddress &) = default;
        MediaAddress(MediaAddress &&) = default;
        MediaAddress &operator=(const MediaAddress &) = default;
        MediaAddress &operator=(MediaAddress &&) = default;

        MediaAddress(T value) : value_{value} {}

        MediaAddress(const etl::array<uint8_t, sizeof(T)> &value)
            : value_{bytes::deserialize<T>(value)} {}

        bool operator==(const MediaAddress &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const MediaAddress &other) const {
            return value_ != other.value_;
        }

        T value() const {
            return value_;
        }

        etl::array<uint8_t, sizeof(T)> serialize() const {
            return bytes::serialize(value_);
        }
    };

    struct UsbSerialAddress : public MediaAddress<uint8_t> {
        using MediaAddress<uint8_t>::MediaAddress;
    };

    template <typename Address>
    class MediaPacket {
        Address source_;
        Address destination_;
        nb::stream::HeapStreamReader<uint8_t> data_;

      public:
        MediaPacket() = delete;
        MediaPacket(const MediaPacket &) = delete;
        MediaPacket(MediaPacket &&) = default;
        MediaPacket &operator=(const MediaPacket &) = delete;
        MediaPacket &operator=(MediaPacket &&) = default;

        MediaPacket(Address source, Address dest, nb::stream::HeapStreamReader<uint8_t> &&data)
            : source_{Address{source}},
              destination_{Address{dest}},
              data_{etl::move(data)} {}

        Address source() const {
            return source_;
        }

        Address destination() const {
            return destination_;
        }

        nb::stream::HeapStreamReader<uint8_t> &data() {
            return data_;
        }
    };

    struct UsbSerialPacket : public MediaPacket<UsbSerialAddress> {
        using MediaPacket<UsbSerialAddress>::MediaPacket;
    };
} // namespace media
