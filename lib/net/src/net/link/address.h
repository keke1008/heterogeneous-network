#pragma once

#include "collection/tiny_buffer.h"
#include "nb/poll.h"
#include "nb/stream.h"
#include "serde/bytes.h"
#include <etl/variant.h>

namespace net::link {
    template <typename T>
    class BaseAddress {
        T value_;

      public:
        BaseAddress() = delete;
        BaseAddress(const BaseAddress &) = default;
        BaseAddress(BaseAddress &&) = default;
        BaseAddress &operator=(const BaseAddress &) = default;
        BaseAddress &operator=(BaseAddress &&) = default;

        inline constexpr BaseAddress(T value) : value_{value} {}

        inline constexpr BaseAddress(const collection::TinyBuffer<uint8_t, sizeof(T)> &buffer)
            : value_{serde::bytes::deserialize<T>(buffer)} {}

        inline constexpr bool operator==(const BaseAddress &other) const {
            return value_ == other.value_;
        }

        inline constexpr bool operator!=(const BaseAddress &other) const {
            return value_ != other.value_;
        }

        inline constexpr T value() const {
            return value_;
        }

        inline constexpr collection::TinyBuffer<uint8_t, sizeof(T)> serialize() const {
            return serde::bytes::serialize(value_);
        }
    };

    class SerialAddress : public BaseAddress<uint8_t> {
        using BaseAddress::BaseAddress;
    };

    class UHFAddress : public BaseAddress<uint8_t> {
        using BaseAddress::BaseAddress;
    };

    template <typename T, typename Address>
    class BaseAddressDeserializer {
        nb::stream::TinyByteWriter<sizeof(T)> writer_;

      public:
        template <typename Reader>
        nb::Poll<Address> poll(Reader &reader) {
            nb::stream::pipe(reader, writer_);
            auto buffer = POLL_UNWRAP_OR_RETURN(writer_.poll());
            T value = serde::bytes::deserialize<T>(buffer);
            return nb::Poll<Address>{Address{value}};
        }
    };

    class SerialAddressDeserializer : public BaseAddressDeserializer<uint8_t, SerialAddress> {};

    class UHFAddressDeserializer : public BaseAddressDeserializer<uint8_t, UHFAddress> {};

    class Address {
        etl::variant<SerialAddress, UHFAddress> value_;

      public:
        uint8_t type() const {
            return value_.index();
        }

        collection::TinyBuffer<uint8_t, 1> serialize() const {
            return etl::visit(
                [&](auto &address) -> collection::TinyBuffer<uint8_t, 1> {
                    return address.serialize();
                },
                value_
            );
        }
    };

    class AddressDeserializer {
        etl::variant<SerialAddressDeserializer, UHFAddressDeserializer> parser_;

      public:
        template <typename Reader>
        nb::Poll<Address> poll(Reader &reader) {
            return etl::visit(
                [&](auto &parser) -> nb::Poll<Address> { return parser.poll(reader); }, parser_
            );
        }
    };

    class AddressSerializer {
        nb::stream::TinyByteReader<1> address_type_;
        nb::stream::TinyByteReader<1> address_;

      public:
        constexpr AddressSerializer(Address &address)
            : address_type_{address.type()},
              address_{address.serialize()} {}

        template <typename Writer>
        inline nb::Poll<nb::Empty> poll(Writer &writer) const {
            nb::stream::pipe_readers(writer, address_type_, address_);
            if (address_.is_reader_closed()) {
                return nb::pending;
            }
            return nb::empty;
        }
    };
} // namespace net::link
