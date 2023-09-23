#pragma once

#include <collection/tiny_buffer.h>
#include <etl/array.h>
#include <etl/variant.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <serde/bytes.h>

namespace net::link {
    enum AddressType : uint8_t {
        Serial = 0,
        UHF = 1,
        IPv4 = 2,
    };

    constexpr inline uint8_t address_length(AddressType type) {
        switch (type) {
        case AddressType::Serial:
            return 1;
        case AddressType::UHF:
            return 1;
        case AddressType::IPv4:
            return 4;
        }
    }

    static inline constexpr uint8_t MAX_ADDRESS_LENGTH = 4;

    class Address {
        AddressType type_;
        etl::array<uint8_t, MAX_ADDRESS_LENGTH> address_;

      public:
        Address() = delete;
        Address(const Address &) = default;
        Address(Address &&) = default;
        Address &operator=(const Address &) = default;
        Address &operator=(Address &&) = default;

        inline Address(AddressType type, etl::span<uint8_t> address) : type_{type} {
            address_.assign(address.begin(), address.end(), 0);
        }

        inline Address(AddressType type, const etl::array<uint8_t, MAX_ADDRESS_LENGTH> &address)
            : type_{type},
              address_{address} {}

        inline Address(AddressType type, etl::array<uint8_t, MAX_ADDRESS_LENGTH> &&address)
            : type_{type},
              address_{address} {}

        constexpr bool operator==(const Address &other) const {
            return type_ == other.type_ && address_ == other.address_;
        }

        constexpr bool operator!=(const Address &other) const {
            return !(*this == other);
        }

        constexpr inline AddressType type() const {
            return type_;
        }

        constexpr inline etl::span<const uint8_t> address() const {
            return etl::span<const uint8_t>{address_.data(), address_length(type_)};
        }

        constexpr uint8_t total_length() const {
            return 1 + address_length(type_);
        }
    };

    class AddressDeserializer final : public nb::stream::WritableBuffer {
        nb::stream::FixedWritableBuffer<1> type_;
        nb::stream::FixedWritableBuffer<4> address_;

        etl::optional<AddressType> type_value_;

      public:
        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            if (!type_value_.has_value()) {
                POLL_UNWRAP_OR_RETURN(type_.write_all_from(source));
                auto type = type_.poll().unwrap();
                type_value_ = static_cast<AddressType>(type[0]);

                auto length = address_length(type_value_.value());
                address_ = nb::stream::FixedWritableBuffer<4>{length};
            }

            return address_.write_all_from(source);
        }

        nb::Poll<Address> poll() {
            auto address = POLL_UNWRAP_OR_RETURN(address_.poll());
            return Address{type_value_.value(), address};
        }
    };

    class AddressSerializer final : public nb::stream::ReadableBuffer {
        nb::stream::FixedReadableBuffer<1 + MAX_ADDRESS_LENGTH> bytes_;

      public:
        AddressSerializer(Address &address)
            : bytes_{static_cast<uint8_t>(address.type()), address.address()} {}

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return bytes_.read_all_into(destination);
        }
    };
} // namespace net::link
