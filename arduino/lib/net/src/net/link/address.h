#pragma once

#include <collection/tiny_buffer.h>
#include <debug_assert.h>
#include <etl/array.h>
#include <etl/initializer_list.h>
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

    constexpr inline uint8_t ADDRESS_TYPE_COUNT = 3;

    class AddressTypeSet {
        uint8_t flags_{0};

        explicit inline constexpr AddressTypeSet(uint8_t flags) : flags_{flags} {}

        static inline constexpr AddressType flags_to_address_type(uint8_t bits) {
            switch (bits) {
            case 0b0001:
                return AddressType::Serial;
            case 0b0010:
                return AddressType::UHF;
            case 0b0100:
                return AddressType::IPv4;
            default:
                DEBUG_ASSERT(false, "Unreachable");
            }
        }

        static inline constexpr int8_t address_type_to_flag(AddressType type) {
            return 1 << static_cast<uint8_t>(type);
        }

        static constexpr uint8_t FLAG_AREA_MASK = 0b0111;

      public:
        AddressTypeSet() = default;
        AddressTypeSet(const AddressTypeSet &) = default;
        AddressTypeSet(AddressTypeSet &&) = default;
        AddressTypeSet &operator=(const AddressTypeSet &) = default;
        AddressTypeSet &operator=(AddressTypeSet &&) = default;

        inline constexpr AddressTypeSet(std::initializer_list<AddressType> type) {
            for (auto t : type) {
                set(t);
            }
        }

        inline constexpr bool operator==(const AddressTypeSet &other) const {
            return flags_ == other.flags_;
        }

        inline constexpr bool operator!=(const AddressTypeSet &other) const {
            return !(*this == other);
        }

        inline constexpr AddressTypeSet operator~() const {
            return AddressTypeSet{static_cast<uint8_t>(~flags_ & FLAG_AREA_MASK)};
        }

        inline constexpr AddressTypeSet operator|(const AddressTypeSet &other) const {
            return AddressTypeSet{static_cast<uint8_t>(flags_ | other.flags_)};
        }

        inline constexpr AddressTypeSet operator&(const AddressTypeSet &other) const {
            return AddressTypeSet{static_cast<uint8_t>(flags_ & other.flags_)};
        }

        inline constexpr AddressTypeSet operator^(const AddressTypeSet &other) const {
            return AddressTypeSet{static_cast<uint8_t>(flags_ ^ other.flags_)};
        }

        inline constexpr AddressTypeSet &operator|=(const AddressTypeSet &other) {
            flags_ |= other.flags_;
            return *this;
        }

        inline constexpr AddressTypeSet &operator&=(const AddressTypeSet &other) {
            flags_ &= other.flags_;
            return *this;
        }

        inline constexpr AddressTypeSet &operator^=(const AddressTypeSet &other) {
            flags_ ^= other.flags_;
            return *this;
        }

        inline constexpr void set(AddressType type) {
            flags_ |= address_type_to_flag(type);
        }

        inline constexpr bool test(AddressType type) const {
            return (flags_ & address_type_to_flag(type)) != 0;
        }

        inline constexpr void reset(AddressType type) {
            uint8_t index = static_cast<uint8_t>(type);
            flags_ &= ~address_type_to_flag(type);
        }

        inline constexpr void flip(AddressType type) {
            flags_ ^= address_type_to_flag(type);
        }

        inline constexpr void flip() {
            flags_ = ~flags_ & FLAG_AREA_MASK;
        }

        inline constexpr bool none() const {
            return flags_ == 0;
        }

        inline constexpr bool any() const {
            return flags_ != 0;
        }

        inline constexpr bool all() const {
            return flags_ == FLAG_AREA_MASK;
        }

        inline etl::optional<AddressType> pick() const {
            if (none()) {
                return etl::nullopt;
            } else {
                uint8_t lsb = flags_ & (~flags_ + 1);
                return flags_to_address_type(lsb);
            }
        }
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
