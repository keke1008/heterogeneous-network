#pragma once

#include <collection/tiny_buffer.h>
#include <debug_assert.h>
#include <etl/array.h>
#include <etl/variant.h>
#include <nb/buf.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <serde/bytes.h>

namespace net::link {
    enum class AddressType : uint8_t {
        Broadcast = 0xff,
        Serial = 0x00,
        UHF = 0x01,
        IPv4 = 0x02,
    };

    constexpr inline uint8_t address_length(AddressType type) {
        switch (type) {
        case AddressType::Broadcast:
            return 0;
        case AddressType::Serial:
            return 1;
        case AddressType::UHF:
            return 1;
        case AddressType::IPv4:
            return 6;
        default: {
            DEBUG_ASSERT(false, "Unreachable");
        }
        }
    }

    static inline constexpr uint8_t MAX_ADDRESS_LENGTH = 6;

    class Address {
        AddressType type_;
        etl::array<uint8_t, MAX_ADDRESS_LENGTH> address_;

      public:
        static constexpr uint8_t MAX_LENGTH = MAX_ADDRESS_LENGTH + 1;

        Address() = delete;
        Address(const Address &) = default;
        Address(Address &&) = default;
        Address &operator=(const Address &) = default;
        Address &operator=(Address &&) = default;

        inline Address(AddressType type, etl::span<const uint8_t> address) : type_{type} {
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

        inline void write_to_builder(nb::buf::BufferBuilder &builder) {
            builder.append(static_cast<uint8_t>(type_));
            builder.append(address());
        }
    };

    struct AddressDeserializer {
        Address parse(nb::buf::BufferSplitter &splitter) {
            auto type = static_cast<AddressType>(splitter.split_1byte());
            auto address = splitter.split_nbytes(address_length(type));
            return Address{type, address};
        }
    };

    class AsyncAddressParser {
        etl::optional<AddressType> type_;
        etl::optional<Address> result_;

      public:
        template <nb::buf::IAsyncBuffer Buffer>
        nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<Buffer> &splitter) {
            if (result_.has_value()) {
                return nb::ready();
            }

            if (!type_.has_value()) {
                uint8_t type = POLL_UNWRAP_OR_RETURN(splitter.split_1byte());
                type_ = static_cast<AddressType>(type);
            }

            uint8_t address_length = net::link::address_length(type_.value());
            auto address = POLL_UNWRAP_OR_RETURN(splitter.split_nbytes(address_length));
            result_ = Address{type_.value(), address};
            return nb::ready();
        }

        inline Address &result() {
            return result_.value();
        }
    };
} // namespace net::link
