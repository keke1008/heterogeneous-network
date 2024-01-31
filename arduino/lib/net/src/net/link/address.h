#pragma once

#include <etl/array.h>
#include <etl/initializer_list.h>
#include <etl/variant.h>
#include <logger.h>
#include <nb/poll.h>
#include <nb/serde.h>

namespace net::link {
    enum class AddressType : uint8_t {
        Serial = 0x01,
        UHF = 0x02,
        Udp = 0x03,
        WebSocket = 0x04,
    };

    constexpr inline bool is_valid_address_type(uint8_t type) {
        return type == static_cast<uint8_t>(AddressType::Serial) ||
            type == static_cast<uint8_t>(AddressType::UHF) ||
            type == static_cast<uint8_t>(AddressType::Udp) ||
            type == static_cast<uint8_t>(AddressType::WebSocket);
    }

    using AsyncAddressTypeDeserializer = nb::de::Enum<AddressType, is_valid_address_type>;
    using AsyncAddressTypeSerializer = nb::ser::Enum<AddressType>;

    constexpr inline uint8_t ADDRESS_TYPE_COUNT = 4;

    constexpr inline uint8_t get_address_body_length_of(AddressType type) {
        switch (type) {
        case AddressType::Serial:
            return 1;
        case AddressType::UHF:
            return 1;
        case AddressType::Udp:
            return 6;
        case AddressType::WebSocket:
            return 6;
        default:
            UNREACHABLE_DEFAULT_CASE;
        }
    }

    static inline constexpr uint8_t MAX_ADDRESS_BODY_LENGTH = 6;

    class AddressTypeIterator {
        uint8_t current_{1};

        explicit inline constexpr AddressTypeIterator(uint8_t current) : current_{current} {}

      public:
        AddressTypeIterator() = default;

        static inline constexpr AddressTypeIterator end() {
            // AddressTypeは1から始まるので、endの値はAddressTypeの数+1で正しい
            return AddressTypeIterator{ADDRESS_TYPE_COUNT + 1};
        }

        inline AddressType operator*() const {
            return static_cast<AddressType>(current_);
        }

        inline constexpr AddressTypeIterator &operator++() {
            current_++;
            return *this;
        }

        inline constexpr bool operator==(const AddressTypeIterator &other) const {
            return current_ == other.current_;
        }

        inline constexpr bool operator!=(const AddressTypeIterator &other) const {
            return !(*this == other);
        }
    };

    class Address {
        AddressType type_;
        etl::array<uint8_t, MAX_ADDRESS_BODY_LENGTH> address_;

      public:
        Address() = delete;
        Address(const Address &) = default;
        Address(Address &&) = default;
        Address &operator=(const Address &) = default;
        Address &operator=(Address &&) = default;

        inline Address(AddressType type, etl::span<const uint8_t> address) : type_{type} {
            FASSERT(address.size() == get_address_body_length_of(type));
            address_.assign(address.begin(), address.end(), 0);
        }

        inline Address(
            AddressType type,
            const etl::array<uint8_t, MAX_ADDRESS_BODY_LENGTH> &address
        )
            : type_{type},
              address_{address} {}

        inline Address(AddressType type, etl::array<uint8_t, MAX_ADDRESS_BODY_LENGTH> &&address)
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

        constexpr inline etl::span<const uint8_t> body() const {
            return etl::span<const uint8_t>{address_.data(), get_address_body_length_of(type_)};
        }

        constexpr uint8_t total_length() const {
            return 1 + get_address_body_length_of(type_);
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const Address &address) {
            printer << static_cast<uint8_t>(address.type()) << '(';
            printer << address.body().subspan(0, get_address_body_length_of(address.type()));
            printer << ')';
            return printer;
        }
    };

    class AsyncAddressSerializer {
        AsyncAddressTypeSerializer address_type_;
        nb::ser::Array<nb::ser::Bin<uint8_t>, MAX_ADDRESS_BODY_LENGTH> address_;

      public:
        explicit AsyncAddressSerializer(Address address)
            : address_type_{address.type()},
              address_{address.body()} {}

        template <nb::ser::AsyncWritable Writable>
        nb::Poll<nb::ser::SerializeResult> serialize(Writable &writable) {
            SERDE_SERIALIZE_OR_RETURN(address_type_.serialize(writable));
            return address_.serialize(writable);
        }

        inline constexpr uint8_t serialized_length() const {
            return address_type_.serialized_length() + address_.serialized_length();
        }

        static inline constexpr uint8_t max_serialized_length() {
            return 1 + MAX_ADDRESS_BODY_LENGTH;
        }
    };

    class AsyncAddressDeserializer {
        AsyncAddressTypeDeserializer address_type_;
        nb::de::Array<nb::de::Bin<uint8_t>, MAX_ADDRESS_BODY_LENGTH> address_{
            nb::de::ARRAY_DUMMY_INITIAL_LENGTH
        };

      public:
        template <nb::de::AsyncReadable Readable>
        nb::Poll<nb::de::DeserializeResult> deserialize(Readable &readable) {
            SERDE_DESERIALIZE_OR_RETURN(address_type_.deserialize(readable));
            address_.set_length(get_address_body_length_of(address_type_.result()));
            return address_.deserialize(readable);
        }

        inline Address result() const {
            return Address{address_type_.result(), address_.result()};
        }
    };

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
                return AddressType::Udp;
            case 0b1000:
                return AddressType::WebSocket;
            default:
                UNREACHABLE_DEFAULT_CASE;
            }
        }

        static inline constexpr int8_t address_type_to_flag(AddressType type) {
            return 1 << (static_cast<uint8_t>(type) - 1);
        }

        static constexpr uint8_t FLAG_AREA_MASK = 0b1111;

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

        friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const AddressTypeSet &set) {
            printer << '[';
            for (uint8_t i = 0; i < ADDRESS_TYPE_COUNT; i++) {
                printer << (set.test(static_cast<AddressType>(i)));
            }
            printer << ']';
            return printer;
        }
    };
} // namespace net::link
