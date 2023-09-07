#pragma once

#include <collection/tiny_buffer.h>
#include <etl/variant.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <serde/bytes.h>

namespace net::link {
    template <uint8_t N>
    class BaseAddress {
        collection::TinyBuffer<uint8_t, N> value_;

      public:
        BaseAddress() = delete;
        BaseAddress(const BaseAddress &) = default;
        BaseAddress(BaseAddress &&) = default;
        BaseAddress &operator=(const BaseAddress &) = default;
        BaseAddress &operator=(BaseAddress &&) = default;

        inline constexpr BaseAddress(const collection::TinyBuffer<uint8_t, N> &buffer)
            : value_{buffer} {}

        inline constexpr bool operator==(const BaseAddress &other) const {
            return value_ == other.value_;
        }

        inline constexpr bool operator!=(const BaseAddress &other) const {
            return value_ != other.value_;
        }

        inline constexpr uint8_t octet_length() const {
            return N;
        }

        inline constexpr const collection::TinyBuffer<uint8_t, N> &serialize() const {
            return value_;
        }
    };

    class SerialAddress : public BaseAddress<1> {
        using BaseAddress::BaseAddress;
    };

    class UHFAddress : public BaseAddress<1> {
        using BaseAddress::BaseAddress;
    };

    class IPv4Address : public BaseAddress<4> {
        using BaseAddress::BaseAddress;
    };

    using AddressVariant = etl::variant<SerialAddress, UHFAddress, IPv4Address>;

    enum class AddressType : uint8_t {
        Serial = 0,
        UHF = 1,
        IPv4 = 2,
    };

    template <typename _Address>
    struct AddressLength {};

    template <>
    struct AddressLength<SerialAddress> {
        static inline constexpr uint8_t value = 1;
    };

    template <>
    struct AddressLength<UHFAddress> {
        static inline constexpr uint8_t value = 1;
    };

    template <>
    struct AddressLength<IPv4Address> {
        static inline constexpr uint8_t value = 4;
    };

    template <typename _Address>
    static inline constexpr uint8_t AddressLength_v = AddressLength<_Address>::value;

    template <typename Address>
    class BaseAddressDeserializer {
        nb::stream::TinyByteWriter<AddressLength_v<Address>> writer_;

      public:
        template <typename Reader>
        nb::Poll<Address> poll(Reader &reader) {
            nb::stream::pipe(reader, writer_);
            auto buffer = POLL_UNWRAP_OR_RETURN(writer_.poll());
            return Address{buffer};
        }
    };

    class SerialAddressDeserializer : public BaseAddressDeserializer<SerialAddress> {};

    class UHFAddressDeserializer : public BaseAddressDeserializer<UHFAddress> {};

    class IPv4AddressDeserializer : public BaseAddressDeserializer<IPv4Address> {};

    class Address {
        AddressVariant value_;

      public:
        AddressType type() const {
            return static_cast<AddressType>(value_.index());
        }

        uint8_t octet_length() const {
            return etl::visit(
                [&](auto &address) -> uint8_t { return address.octet_length() + 1; }, value_
            );
        }
    };

    class AddressDeserializer {
        nb::stream::TinyByteWriter<1> address_type_;
        etl::variant<
            etl::monostate,
            SerialAddressDeserializer,
            UHFAddressDeserializer,
            IPv4AddressDeserializer>
            parser_;

      public:
        template <typename Reader>
        nb::Poll<Address> poll(Reader &reader) {
            if (etl::holds_alternative<etl::monostate>(parser_)) {
                auto address_type = POLL_UNWRAP_OR_RETURN(address_type_.poll(reader));
                switch (address_type) {
                case AddressType::Serial:
                    parser_ = SerialAddressDeserializer{};
                    break;
                case AddressType::UHF:
                    parser_ = UHFAddressDeserializer{};
                    break;
                case AddressType::IPv4:
                    parser_ = IPv4AddressDeserializer{};
                    break;
                }
            }

            nb::stream::pipe(reader, address_type_);
            return etl::visit(
                [&](auto &deserializer) -> nb::Poll<Address> {
                    auto address = POLL_UNWRAP_OR_RETURN(deserializer.poll(reader));
                    return Address{address};
                },
                parser_
            );
        }
    };

    class AddressSerializer {
        nb::stream::TinyByteReader<1> address_type_;
        nb::stream::TinyByteReader<1> address_;

      public:
        constexpr AddressSerializer(Address &address)
            : address_type_{static_cast<uint8_t>(address.type())},
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
