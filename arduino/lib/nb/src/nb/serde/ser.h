#pragma once

#include <etl/limits.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/vector.h>
#include <nb/poll.h>
#include <util/concepts.h>

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
    };

    template <util::integral T>
    class Bin {
        T value_;
        bool done_{false};

      public:
        inline explicit Bin(T value) : value_{value} {}

        template <AsyncWritable Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            if (done_) {
                return SerializeResult::Ok;
            }

            constexpr uint8_t length = sizeof(T);
            SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(length));
            for (uint8_t i = 0; i < length; i++) {
                writable.write_unchecked(value_ & 0xFF);
                value_ >>= 8;
            }

            done_ = true;
            return SerializeResult::Ok;
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
    };

    template <util::integral T>
    class Hex {
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

            static constexpr uint8_t length = sizeof(T) * 2;
            SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(length));
            for (uint8_t i = length; i > 0; i -= 2) {
                uint8_t byte = (value_ >> (i - 2) * 4) & 0xFF;
                writable.write_unchecked(to_hex_char(byte >> 4));
                writable.write_unchecked(to_hex_char(byte & 0xF));
            }

            return SerializeResult::Ok;
        }
    };

    template <util::integral T>
    class Dec {
        T value_;
        T base_{pow10(etl::numeric_limits<T>::digits10)};
        bool is_in_leading_zeros_{true};

        inline constexpr T pow10(T power) {
            return power == 0 ? 1 : 10 * pow10(power - 1);
        }

      public:
        explicit constexpr Dec(T value) : value_{value} {}

        template <AsyncWritable Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            for (; base_ != 0; base_ /= 10) {
                SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(1));

                T div = value_ / base_;
                value_ -= div * base_;

                if (div != 0 || !is_in_leading_zeros_) {
                    writable.write_unchecked(div + '0');
                    is_in_leading_zeros_ = false;
                }
            }

            return SerializeResult::Ok;
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
    };

    template <typename Serializable, uint8_t N>
    class Vec {
        Bin<uint8_t> length_;
        etl::vector<Serializable, N> vec_{};
        uint8_t index_{0};

      public:
        template <uint8_t M>
        explicit Vec(etl::span<Serializable, M> span)
            : length_{span.size()},
              vec_{span.begin(), span.end()} {
            static_assert(M <= N);
        }

        explicit Vec(etl::span<Serializable> span)
            : length_{span.size()},
              vec_{span.begin(), span.end()} {
            ASSERT(span.size() <= N);
        }

        template <typename T, uint8_t M>
        explicit Vec(etl::span<const T, M> span) : length_{static_cast<uint8_t>(span.size())} {
            static_assert(M <= N);
            for (const auto &item : span) {
                vec_.push_back(Serializable{item});
            }
        }

        template <typename T>
        explicit Vec(etl::span<const T> span) : length_{static_cast<uint8_t>(span.size())} {
            ASSERT(span.size() <= N);
            for (const auto &item : span) {
                vec_.push_back(Serializable{item});
            }
        }

        template <typename T, size_t M>
        explicit Vec(const etl::array<T, M> &array) : length_{static_cast<uint8_t>(array.size())} {
            static_assert(M <= N);
            for (const auto &item : array) {
                vec_.push_back(Serializable{item});
            }
        }

        template <typename T, size_t M>
        explicit Vec(const etl::vector<T, M> &vector)
            : length_{static_cast<uint8_t>(vector.size())} {
            static_assert(M <= N);
            for (const auto &item : vector) {
                vec_.push_back(Serializable{item});
            }
        }

        template <AsyncWritable Writable>
            requires AsyncSerializable<Serializable, Writable>
        nb::Poll<SerializeResult> serialize(Writable &writable) {
            if (index_ == N) {
                return SerializeResult::Ok;
            }

            SERDE_SERIALIZE_OR_RETURN(length_.serialize(writable));
            for (; index_ < N; index_++) {
                SERDE_SERIALIZE_OR_RETURN(vec_[index_].serialize(writable));
            }

            return SerializeResult::Ok;
        }
    };
} // namespace nb::ser
