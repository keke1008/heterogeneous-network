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

        inline constexpr const collection::TinyBuffer<uint8_t, N> &value() const {
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

    inline constexpr uint8_t address_length(AddressType type) {
        switch (type) {
        case AddressType::Serial:
            return 1;
        case AddressType::UHF:
            return 1;
        case AddressType::IPv4:
            return 4;
        }
    }

    class Address {
        AddressType type_;
        collection::TinyBuffer<uint8_t, 4> value_{0, 0, 0, 0};

      public:
        constexpr Address(AddressType type, const collection::TinyBuffer<uint8_t, 4> &value)
            : type_{type},
              value_{value} {}

        constexpr AddressType type() const {
            return type_;
        }

        constexpr const collection::TinyBuffer<uint8_t, 4> &value() const {
            return value_;
        }
    };

    class AddressDeserializer {
        int8_t address_value_index_{-1};
        AddressType type_;
        collection::TinyBuffer<uint8_t, 4> value_;

      public:
        template <typename Reader>
        nb::Poll<Address> poll(Reader &reader) {
            if (address_value_index_ == -1) {
                auto type = reader.read();
                if (!type.has_value()) {
                    return nb::pending;
                }
                type_ = static_cast<AddressType>(type.value());
                address_value_index_++;
            }

            auto length = address_length(type_);
            for (; address_value_index_ < length; address_value_index_++) {
                auto byte = reader.read();
                if (!byte.has_value()) {
                    return nb::pending;
                }
                value_[address_value_index_] = byte.value();
            }

            return Address{type_, value_};
        }
    };

    class AddressSerializer {
        int8_t state_{-1};
        AddressType type_;
        collection::TinyBuffer<uint8_t, 4> value_;

      public:
        constexpr AddressSerializer(Address &address)
            : type_{address.type()},
              value_{address.value()} {}

        template <typename Writer>
        inline nb::Poll<nb::Empty> poll(Writer &writer) {
            if (state_ == -1) {
                writer.write(static_cast<uint8_t>(type_));
                state_++;
            }

            auto length = address_length(type_);
            for (; state_ < length; state_++) {
                if (!writer.write(value_[state_])) {
                    return nb::pending;
                }
            }
            return nb::empty;
        }
    };
} // namespace net::link
