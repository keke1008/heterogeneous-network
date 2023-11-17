#pragma once

#include <etl/optional.h>
#include <etl/span.h>
#include <etl/vector.h>
#include <logger.h>
#include <nb/poll.h>
#include <util/concepts.h>

#define SERDE_DESERIALIZE_OR_RETURN(result)                                                        \
    do {                                                                                           \
        auto _result = POLL_UNWRAP_OR_RETURN(result);                                              \
        if (_result != nb::de::DeserializeResult::Ok) {                                            \
            return _result;                                                                        \
        }                                                                                          \
    } while (0)

namespace nb::de {
    enum class DeserializeResult : uint8_t {
        Ok,
        NotEnoughLength,
        Invalid,
    };

    template <typename T>
    concept AsyncReadable = requires(T &readable, uint8_t read_count) {
        { readable.poll_readable(read_count) } -> util::same_as<nb::Poll<DeserializeResult>>;
        { readable.read() } -> util::same_as<nb::Poll<etl::pair<uint8_t, DeserializeResult>>>;
        { readable.read_unchecked() } -> util::same_as<uint8_t>;
    };

    template <typename T, typename Readable>
    concept AsyncDeserializable = AsyncReadable<Readable> && requires(T &de, Readable &readable) {
        { T{} } -> util::same_as<T>;
        { de.deserialize(readable) } -> util::same_as<nb::Poll<DeserializeResult>>;
        { de.result() };
    };

    template <typename T>
    using DeserializeResultType = etl::decay_t<decltype(etl::declval<T>().result())>;

    template <util::integral T>
    class Bin {
        constexpr static uint8_t length = sizeof(T);

        T value_{0};
        bool done_{false};

      public:
        inline T result() const {
            return value_;
        }

        template <AsyncReadable Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            if (done_) {
                return DeserializeResult::Ok;
            }

            SERDE_DESERIALIZE_OR_RETURN(readable.poll_readable(length));
            for (uint8_t i = 0; i < length; ++i) {
                auto byte = readable.read_unchecked();
                value_ |= static_cast<T>(byte) << (i * 8);
            }

            done_ = true;
            return DeserializeResult::Ok;
        }
    };

    class Bool {
        Bin<uint8_t> bin_;

      public:
        inline bool result() const {
            return bin_.result() != 0;
        }

        template <AsyncReadable Readable>
        inline nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            return bin_.deserialize(readable);
        }
    };

    template <util::integral T>
    class Hex {
        inline constexpr static uint8_t length = sizeof(T) * 2;

        T value_{0};
        bool done_{false};

        static constexpr inline bool is_valid_char(uint8_t c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        }

        static constexpr inline uint8_t from_hex_char(uint8_t c) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            } else if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            } else {
                PANIC("Invalid hex char");
            }
        }

      public:
        inline T result() const {
            return value_;
        }

        template <AsyncReadable Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            if (done_) {
                return DeserializeResult::Ok;
            }

            SERDE_DESERIALIZE_OR_RETURN(readable.poll_readable(length));
            for (uint8_t remaining_ = length; remaining_ > 0; --remaining_) {
                auto byte = readable.read_unchecked();
                if (!is_valid_char(byte)) {
                    return DeserializeResult::Invalid;
                }

                value_ |= static_cast<T>(from_hex_char(byte)) << ((remaining_ - 1) * 4);
            }

            return DeserializeResult::Ok;
        }
    };

    template <util::integral T>
    class Dec {
        bool done_{false};
        bool read_once_{false};
        T value_{0};

      public:
        inline T result() const {
            return value_;
        }

        template <AsyncReadable Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            if (done_) {
                return DeserializeResult::Ok;
            }

            while (true) {
                auto [dest, result] = POLL_UNWRAP_OR_RETURN(readable.read());
                if (result != DeserializeResult::Ok) {
                    done_ = true;
                    return read_once_ ? DeserializeResult::Ok : DeserializeResult::NotEnoughLength;
                }
                read_once_ = true;

                if (dest < '0' || dest > '9') {
                    return DeserializeResult::Invalid;
                }

                value_ *= 10;
                value_ += dest - '0';
            }
        }
    };

    template <typename Deserializable>
    class Optional {
        Bool has_value_;
        Deserializable value_;
        bool done_{false};

      public:
        explicit Optional() = default;

        explicit Optional(Deserializable &value) : has_value_{value.has_value()}, value_{value} {}

        explicit Optional(Deserializable &&value)
            : has_value_{value.has_value()},
              value_{etl::move(value)} {}

        inline const etl::optional<DeserializeResultType<Deserializable>> result() const {
            return has_value_.result() ? etl::optional{value_.result()} : etl::nullopt;
        }

        template <AsyncReadable Readable>
            requires AsyncDeserializable<Deserializable, Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            if (done_) {
                return DeserializeResult::Ok;
            }

            SERDE_DESERIALIZE_OR_RETURN(has_value_.deserialize(readable));

            if (has_value_.result()) {
                SERDE_DESERIALIZE_OR_RETURN(value_.deserialize(readable));
            }

            done_ = true;
            return DeserializeResult::Ok;
        }
    };

    template <typename Deserializable, uint8_t N>
    class Vec {
        Bin<uint8_t> length_;
        Deserializable deserializable_{};
        etl::vector<DeserializeResultType<Deserializable>, N> vector_;

      public:
        inline etl::span<etl::add_const_t<DeserializeResultType<Deserializable>>> result() const {
            return etl::span{vector_.begin(), length_.result()};
        }

        template <AsyncReadable Readable>
            requires AsyncDeserializable<Deserializable, Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            SERDE_DESERIALIZE_OR_RETURN(length_.deserialize(readable));

            while (vector_.size() < length_.result()) {
                SERDE_DESERIALIZE_OR_RETURN(deserializable_.deserialize(readable));
                vector_.push_back(deserializable_.result());
                deserializable_ = Deserializable{};
            }

            return DeserializeResult::Ok;
        }
    };
} // namespace nb::de
