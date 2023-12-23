#pragma once

#include "./common.h"
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/variant.h>
#include <etl/vector.h>
#include <nb/poll.h>
#include <tl/tuple.h>
#include <util/concepts.h>
#include <util/span.h>

#define SERDE_SERIALIZE_OR_RETURN(result)                                                          \
    do {                                                                                           \
        auto _result = POLL_UNWRAP_OR_RETURN(result);                                              \
        if (_result != nb::ser::SerializeResult::Ok) {                                             \
            return _result;                                                                        \
        }                                                                                          \
    } while (0)

namespace nb::ser {
    enum class SerializeResult : uint8_t {
        Ok,
        NotEnoughLength,
    };

    template <typename T>
    concept AsyncWritable = requires(T &writable, uint8_t write_count, uint8_t byte) {
        { writable.poll_writable(write_count) } -> util::same_as<nb::Poll<SerializeResult>>;
        { writable.write(byte) } -> util::same_as<nb::Poll<SerializeResult>>;
        { writable.write_unchecked(byte) } -> util::same_as<void>;
    };

    template <typename T, typename Writable>
    concept AsyncSerializable = AsyncWritable<Writable> && requires(T &ser, Writable &writable) {
        { ser.serialize(writable) } -> util::same_as<nb::Poll<SerializeResult>>;
        { ser.serialized_length() } -> util::same_as<uint8_t>;
    };

    template <util::integral T>
    class Bin {
        static constexpr uint8_t length = sizeof(T);
        T value_;
        bool done_{false};

      public:
        inline explicit Bin(T value) : value_{static_cast<T>(value)} {}

        template <AsyncWritable Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            if (done_) {
                return SerializeResult::Ok;
            }

            SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(length));
            for (uint8_t i = 0; i < length; i++) {
                writable.write_unchecked(value_ & 0xFF);
                value_ >>= 8;
            }

            done_ = true;
            return SerializeResult::Ok;
        }

        inline constexpr uint8_t serialized_length() const {
            return length;
        }
    };

    class Bool {
        Bin<uint8_t> bin_;

      public:
        inline explicit Bool(bool value) : bin_{static_cast<uint8_t>(value)} {}

        template <AsyncWritable Writable>
        inline nb::Poll<SerializeResult> serialize(Writable &writable) {
            return bin_.serialize(writable);
        }

        inline constexpr uint8_t serialized_length() const {
            return 1;
        }
    };

    template <util::integral T>
    class Hex {
        static constexpr uint8_t length = sizeof(T) * 2;
        T value_;
        bool done_{false};

        static inline constexpr uint8_t to_hex_char(uint8_t value) {
            return value < 10 ? value + '0' : value - 10 + 'A';
        }

      public:
        explicit constexpr Hex(T value) : value_{value} {}

        template <AsyncWritable Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            if (done_) {
                return SerializeResult::Ok;
            }

            SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(length));
            for (uint8_t i = length; i > 0; i -= 2) {
                uint8_t byte = (value_ >> (i - 2) * 4) & 0xFF;
                writable.write_unchecked(to_hex_char(byte >> 4));
                writable.write_unchecked(to_hex_char(byte & 0xF));
            }

            done_ = true;
            return SerializeResult::Ok;
        }

        inline constexpr uint8_t serialized_length() const {
            return length;
        }
    };

    template <util::integral T>
    class Dec {
        T value_;
        T base_;
        uint8_t length_;

      public:
        explicit constexpr Dec(T value) : value_{value} {
            if (value_ == 0) {
                length_ = 1;
                base_ = 1;
            } else {
                length_ = 1;
                base_ = 1;
                for (T div = value_ / 10; div > 0; div /= 10) {
                    base_ *= 10;
                    length_++;
                }
            }
        }

        template <AsyncWritable Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            for (; base_ != 0; base_ /= 10) {
                SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(1));
                T div = value_ / base_;
                value_ -= div * base_;
                writable.write_unchecked(div + '0');
            }

            return SerializeResult::Ok;
        }

        inline constexpr uint8_t serialized_length() const {
            return length_;
        }
    };

    template <typename E>
    class Enum {
        Bin<util::underlying_type_t<E>> value_;

      public:
        explicit Enum(E value) : value_{static_cast<util::underlying_type_t<E>>(value)} {}

        template <AsyncWritable Writable>
        inline nb::Poll<SerializeResult> serialize(Writable &writable) {
            return value_.serialize(writable);
        }

        inline constexpr uint8_t serialized_length() const {
            return value_.serialized_length();
        }
    };

    template <typename Serializable>
    class Optional {
        Bool has_value_;
        etl::optional<Serializable> value_;
        bool done_{false};

      public:
        explicit Optional(const Serializable &value) : has_value_{true}, value_{value} {}

        explicit Optional(const etl::optional<Serializable> &value)
            : has_value_{value.has_value()},
              value_{value.has_value() ? etl::optional{*value} : etl::nullopt} {}

        template <typename T>
        explicit Optional(const etl::optional<T> &value)
            : has_value_{value.has_value()},
              value_{value.has_value() ? etl::optional{Serializable{*value}} : etl::nullopt} {}

        template <typename T>
        explicit Optional(const T &value) : has_value_{true},
                                            value_{Serializable{value}} {}

        template <AsyncWritable Writable>
            requires AsyncSerializable<Serializable, Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            if (done_) {
                return SerializeResult::Ok;
            }

            SERDE_SERIALIZE_OR_RETURN(has_value_.serialize(writable));
            if (value_.has_value()) {
                SERDE_SERIALIZE_OR_RETURN(value_->serialize(writable));
            }

            done_ = true;
            return SerializeResult::Ok;
        }

        inline constexpr uint8_t serialized_length() const {
            return has_value_.serialized_length() +
                (value_.has_value() ? value_->serialized_length() : 0);
        }
    };

    template <typename Serializable, uint8_t MAX_LENGTH>
    class Array {
        etl::vector<Serializable, MAX_LENGTH> vector_;
        uint8_t index_{0};

      public:
        template <uint8_t N>
        explicit Array(etl::span<Serializable, N> span) : vector_{span.begin(), span.end()} {
            static_assert(N <= MAX_LENGTH);
        }

        explicit Array(etl::span<Serializable> span) : vector_{span.begin(), span.end()} {
            FASSERT(span.size() <= MAX_LENGTH);
        }

        template <typename T>
        explicit Array(etl::span<const T, MAX_LENGTH> span) {
            for (const auto &item : span) {
                vector_.push_back(Serializable{item});
            }
        }

        template <typename T>
        explicit Array(etl::span<const T> span) {
            FASSERT(span.size() <= MAX_LENGTH);
            for (const auto &item : span) {
                vector_.push_back(Serializable{item});
            }
        }

        template <typename T, size_t N>
        explicit Array(const etl::array<T, N> &array) {
            static_assert(N <= MAX_LENGTH);
            for (const auto &item : array) {
                vector_.push_back(Serializable{item});
            }
        }

        template <typename T, size_t N>
        explicit Array(const etl::vector<T, N> &vector) {
            static_assert(N <= MAX_LENGTH);
            for (const auto &item : vector) {
                vector_.push_back(Serializable{item});
            }
        }

        inline constexpr uint8_t length() const {
            return vector_.size();
        }

        inline constexpr uint8_t serialized_length() const {
            uint8_t length = 0;
            for (const auto &item : vector_) {
                length += item.serialized_length();
            }
            return length;
        }

        template <AsyncWritable Writable>
            requires AsyncSerializable<Serializable, Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            if (index_ == vector_.size()) {
                return SerializeResult::Ok;
            }

            for (; index_ < vector_.size(); index_++) {
                SERDE_SERIALIZE_OR_RETURN(vector_[index_].serialize(writable));
            }

            return SerializeResult::Ok;
        }
    };

    template <typename Serializable, uint8_t N>
    class Vec {
        Array<Serializable, N> array_;
        Bin<uint8_t> length_;

      public:
        template <typename... Args>
        explicit Vec(Args &&...args)
            : array_{etl::forward<Args>(args)...},
              length_{array_.length()} {}

        inline constexpr uint8_t serialized_length() const {
            return length_.serialized_length() + array_.serialized_length();
        }

        template <AsyncWritable Writable>
            requires AsyncSerializable<Serializable, Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            SERDE_SERIALIZE_OR_RETURN(length_.serialize(writable));
            return array_.serialize(writable);
        }
    };

    template <typename... Serializables>
    class Union {
        etl::variant<Serializables...> variant_;

        template <typename... Ts>
        struct Visitor {
            template <typename T>
            etl::variant<Serializables...> operator()(T &&value) {
                static constexpr size_t I = tl::index_of_v<etl::decay_t<T>, Ts...>;
                using Serializable = tl::type_index_t<I, Serializables...>;
                return Serializable{etl::forward<T>(value)};
            }
        };

        template <typename... Ts>
        static etl::variant<Serializables...> visited(const etl::variant<Ts...> &variant) {
            return etl::visit(Visitor<Ts...>{}, variant);
        }

      public:
        template <typename T>
        explicit Union(const T &value) : variant_{value} {}

        template <typename... Ts>
        explicit Union(const etl::variant<Ts...> &variant) : variant_{visited(variant)} {}

        template <AsyncWritable Writable>
            requires(AsyncSerializable<Serializables, Writable> && ...)
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            return etl::visit(
                [&writable](auto &value) -> nb::Poll<SerializeResult> {
                    return value.serialize(writable);
                },
                variant_
            );
        }

        inline constexpr uint8_t serialized_length() const {
            return etl::visit(
                [](const auto &value) -> uint8_t { return value.serialized_length(); }, variant_
            );
        }
    };

    template <typename... Serializables>
    class Variant {
        Bin<uint8_t> index_;
        Union<Serializables...> union_;

      public:
        template <typename T>
        explicit Variant(const T &value)
            : index_{tl::index_of_v<etl::decay_t<T>, Serializables...> + 1},
              union_{value} {}

        template <typename... Ts>
        explicit Variant(const etl::variant<Ts...> &variant)
            : index_{static_cast<uint8_t>(variant.index() + 1)},
              union_{variant} {}

        template <AsyncWritable Writable>
            requires(AsyncSerializable<Serializables, Writable> && ...)
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            SERDE_SERIALIZE_OR_RETURN(index_.serialize(writable));
            return union_.serialize(writable);
        }

        inline constexpr uint8_t serialized_length() const {
            return index_.serialized_length() + union_.serialized_length();
        }
    };

    class Empty {
      public:
        explicit Empty() = default;

        explicit Empty(const EmptyMarker &) {}

        template <AsyncWritable Writable>
        inline nb::Poll<SerializeResult> serialize(Writable &writable) {
            return SerializeResult::Ok;
        }

        inline constexpr uint8_t serialized_length() const {
            return 0;
        }
    };

    class AsyncStaticSpanSerializer {
        etl::span<const uint8_t> span_;
        uint8_t index_{0};

      public:
        explicit AsyncStaticSpanSerializer(etl::span<const uint8_t> span) : span_{span} {}

        explicit AsyncStaticSpanSerializer(etl::string_view string)
            : span_{util::as_bytes(string)} {}

        template <AsyncWritable Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            while (index_ < span_.size()) {
                SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(1));
                writable.write_unchecked(span_[index_++]);
            }
            return SerializeResult::Ok;
        }

        inline constexpr uint8_t serialized_length() const {
            return span_.size();
        }
    };

    template <uint8_t MAX_LENGTH>
    class AsyncSpanSerializer {
        etl::array<uint8_t, MAX_LENGTH> array_;
        AsyncStaticSpanSerializer serializer_;

      public:
        explicit AsyncSpanSerializer(etl::span<const uint8_t> span)
            : array_{},
              serializer_{etl::span{array_.begin(), span.size()}} {
            array_.assign(span.begin(), span.end());
        }

        template <typename... Args>
        explicit AsyncSpanSerializer(Args &&...args)
            : array_{},
              serializer_{etl::span{array_.begin(), sizeof...(Args)}} {
            auto ilist = {etl::forward<Args>(args)...};
            array_.assign(ilist.begin(), ilist.end());
        }

        template <AsyncWritable Writable>
        inline nb::Poll<SerializeResult> serialize(Writable &writable) {
            return serializer_.serialize(writable);
        }

        inline constexpr uint8_t serialized_length() const {
            return serializer_.serialized_length();
        }
    };
} // namespace nb::ser
