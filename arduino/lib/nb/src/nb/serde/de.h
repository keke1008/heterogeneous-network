#pragma once

#include "./common.h"
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/variant.h>
#include <etl/vector.h>
#include <logger.h>
#include <nb/poll.h>
#include <tl/tuple.h>
#include <util/concepts.h>
#include <util/visitor.h>

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
    concept AsyncReadable = requires(T &readable, uint8_t read_count, uint8_t &dest) {
        { readable.poll_readable(read_count) } -> util::same_as<nb::Poll<DeserializeResult>>;
        { readable.read(dest) } -> util::same_as<nb::Poll<DeserializeResult>>;
        { readable.read_unchecked() } -> util::same_as<uint8_t>;
    };

    template <typename T>
    concept AsyncBufferedReadable = AsyncReadable<T> && requires(T &readable, uint8_t length) {
        { readable.read_span_unchecked(length) } -> util::same_as<etl::span<const uint8_t>>;
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

    /**
     * 10進数の数値をデシリアライズする．
     *
     * 0~9までの文字が1回以上出現し，これ以上読み込めないor数字以外の文字が出現したら終了する．
     * 数字以外の文字の出現により終了した場合，その文字を読み込んだ状態で終了する．
     *
     * # Example
     *
     * * "123" -> 123
     * * "123abc" -> 123
     * * "abc" -> NotEnoughLength
     */
    template <util::integral T>
    class Dec {
        bool done_{false};
        bool read_at_least_once_{false};
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
                uint8_t byte;
                auto result = POLL_UNWRAP_OR_RETURN(readable.read(byte));
                if (result != DeserializeResult::Ok || byte < '0' || byte > '9') {
                    done_ = true;
                    return read_at_least_once_ ? DeserializeResult::Ok
                                               : DeserializeResult::NotEnoughLength;
                }
                read_at_least_once_ = true;

                value_ *= 10;
                value_ += byte - '0';
            }
        }
    };

    template <typename E, auto (*Validator)(util::underlying_type_t<E>)->bool>
    class Enum {
        Bin<util::underlying_type_t<E>> value_;

      public:
        inline E result() const {
            return static_cast<E>(value_.result());
        }

        template <AsyncReadable Readable>
        inline nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            SERDE_DESERIALIZE_OR_RETURN(value_.deserialize(readable));
            return Validator(value_.result()) ? DeserializeResult::Ok : DeserializeResult::Invalid;
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

    struct ArrayDummyInitialLength {};

    // コンストラクタで初期化する際，ダミーの値であることを明示的に示す．
    inline constexpr ArrayDummyInitialLength ARRAY_DUMMY_INITIAL_LENGTH{};

    template <typename Deserializable, uint8_t MAX_LENGTH>
    class Array {
        Deserializable deserializable_{};
        etl::vector<DeserializeResultType<Deserializable>, MAX_LENGTH> vector_;
        uint8_t length_;

      public:
        explicit Array() = delete;

        explicit constexpr Array(uint8_t length) : length_{length} {}

        static inline constexpr uint8_t DUMMY_INITIAL_LENGTH = 0;

        explicit constexpr Array(ArrayDummyInitialLength) : length_{DUMMY_INITIAL_LENGTH} {}

        inline void set_length(uint8_t length) {
            if (length != length_) {
                ASSERT(vector_.size() == 0);
                length_ = length;
            }
        }

        inline etl::vector<etl::decay_t<DeserializeResultType<Deserializable>>, MAX_LENGTH>
        result() const {
            return vector_;
        }

        template <AsyncReadable Readable>
            requires AsyncDeserializable<Deserializable, Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            while (vector_.size() < length_) {
                SERDE_DESERIALIZE_OR_RETURN(deserializable_.deserialize(readable));
                vector_.push_back(deserializable_.result());
                deserializable_ = Deserializable{};
            }

            return DeserializeResult::Ok;
        }
    };

    template <typename Deserializable, uint8_t MAX_LENGTH>
    class Vec {
        Bin<uint8_t> length_;
        Array<Deserializable, MAX_LENGTH> array_{ARRAY_DUMMY_INITIAL_LENGTH};

      public:
        inline etl::vector<etl::decay_t<DeserializeResultType<Deserializable>>, MAX_LENGTH>
        result() const {
            return array_.result();
        }

        template <AsyncReadable Readable>
            requires AsyncDeserializable<Deserializable, Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            SERDE_DESERIALIZE_OR_RETURN(length_.deserialize(readable));
            array_.set_length(length_.result());
            return array_.deserialize(readable);
        }
    };

    template <typename... Deserializables>
        requires(sizeof...(Deserializables) > 0)
    class Union {
        etl::variant<Deserializables...> variant_;

      public:
        constexpr Union() = delete;

        template <typename T>
        explicit Union(T &&t) : variant_{etl::forward<T>(t)} {}

        using Result = etl::variant<DeserializeResultType<Deserializables>...>;

        inline Result result() const {
            return etl::visit<Result>([](auto &de) { return de.result(); }, variant_);
        }

        template <AsyncReadable Readable>
            requires(AsyncDeserializable<Deserializables, Readable> && ...)
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            return etl::visit<nb::Poll<DeserializeResult>>(
                [&readable](auto &de) { return de.deserialize(readable); }, variant_
            );
        }
    };

    template <typename... Deserializables>
        requires(sizeof...(Deserializables) > 0)
    class Variant {
        Bin<uint8_t> index_;
        etl::variant<etl::monostate, Deserializables...> union_{};

        using VTableEntry = auto (*)() -> etl::variant<etl::monostate, Deserializables...>;
        using VTable = VTableEntry[sizeof...(Deserializables) + 1];

        static constexpr VTable vtable{
            []() { return etl::monostate{}; },
            ([]() { return Deserializables{}; }, ...),
        };

      public:
        template <AsyncReadable Readable>
            requires(AsyncDeserializable<Deserializables, Readable> && ...)
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            if (etl::holds_alternative<etl::monostate>(union_)) {
                SERDE_DESERIALIZE_OR_RETURN(index_.deserialize(readable));
                auto index = index_.result();
                if (index == 0 || index >= sizeof...(Deserializables)) {
                    return DeserializeResult::Invalid;
                }

                union_ = vtable[index]();
            }

            return etl::visit<nb::Poll<DeserializeResult>>(
                util::Visitor{
                    [](etl::monostate) { return DeserializeResult::Invalid; },
                    [&readable](auto &de) { return de.deserialize(readable); },
                },
                union_
            );
        }

        using Result = etl::variant<DeserializeResultType<Deserializables>...>;

        inline Result result() const {
            ASSERT(!etl::holds_alternative<etl::monostate>(union_));
            return etl::visit<Result>(
                util::Visitor{
                    [](etl::monostate) -> Result { PANIC("Invalid state to call result()"); },
                    [](auto &de) -> Result { return de.result(); },
                },
                union_
            );
        }
    };

    class Empty {
      public:
        inline constexpr EmptyMarker result() const {
            return EmptyMarker{};
        }

        template <AsyncReadable Readable>
        inline nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            return DeserializeResult::Ok;
        }
    };

    class SkipNBytes {
        uint8_t count_;

      public:
        explicit constexpr SkipNBytes(uint8_t remaining) : count_{remaining} {}

        inline constexpr void result() const {}

        template <AsyncReadable Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            SERDE_DESERIALIZE_OR_RETURN(readable.poll_readable(count_));
            for (uint8_t i = 0; i < count_; ++i) {
                readable.read_unchecked();
            }
            return DeserializeResult::Ok;
        }
    };

    class AsyncDiscardingSingleLineDeserializer {
        bool is_last_cr_{false};
        bool done_{false};

      public:
        explicit constexpr AsyncDiscardingSingleLineDeserializer() = default;

        explicit constexpr AsyncDiscardingSingleLineDeserializer(uint8_t last_char)
            : is_last_cr_{last_char == '\r'} {}

        inline constexpr void result() const {}

        template <AsyncReadable Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            if (done_) {
                return DeserializeResult::Ok;
            }

            while (true) {
                uint8_t byte;
                SERDE_DESERIALIZE_OR_RETURN(readable.read(byte));

                if (is_last_cr_ && byte == '\n') {
                    done_ = true;
                    return DeserializeResult::Ok;
                }

                is_last_cr_ = byte == '\r';
            }
        }
    };

    inline bool is_complete_line(etl::span<const uint8_t> line) {
        if (line.size() < 2) {
            return false;
        }
        auto last = line.end() - 1;
        return *(last - 1) == '\r' && *last == '\n';
    }

    /**
     * 最大`MAX_LENGTH`の長さの1行をデシリアライズする．
     * `MAX_LENGTH`を超える文字列は自動で捨てられる．
     * 1行(\r\n)が書き込まれると，行の長さに関わらずready()を返す．
     *
     * `MAX_LENGTH`を超えなった場合にのみ，`DeserializeResult::Ok`を返す．
     */
    template <uint8_t MAX_LENGTH>
    class AsyncMaxLengthSingleLineBytesDeserializer {
        static_assert(MAX_LENGTH >= 2, "buffer must be able to contain CRLF");

        etl::vector<uint8_t, MAX_LENGTH> buffer_{};
        etl::optional<AsyncDiscardingSingleLineDeserializer> discarding_deserializer_{};

        inline bool is_complete() const {
            return is_complete_line(buffer_);
        }

      public:
        inline void reset() {
            buffer_.clear();
            discarding_deserializer_.reset();
        }

        inline etl::span<const uint8_t> written_bytes() const {
            return etl::span{buffer_.begin(), buffer_.size()};
        }

        inline etl::span<const uint8_t> result() const {
            ASSERT(is_complete());
            return etl::span{buffer_.begin(), buffer_.size()};
        }

        template <AsyncReadable Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            if (is_complete()) {
                return DeserializeResult::Ok;
            }

            while (!buffer_.full()) {
                uint8_t byte;
                SERDE_DESERIALIZE_OR_RETURN(readable.read(byte));

                buffer_.push_back(byte);
                if (is_complete()) {
                    return DeserializeResult::Ok;
                }
            }

            if (!discarding_deserializer_.has_value()) {
                discarding_deserializer_.emplace(buffer_.back());
            }

            SERDE_DESERIALIZE_OR_RETURN(discarding_deserializer_->deserialize(readable));
            return DeserializeResult::Invalid;
        }
    };

    /**
     * 少なくとも`MIN_LENGTH`の長さは持つ一行を書き込むバッファ．
     *
     * `MIN_LENGTH - 1`byte書き込まれる中に\r\nが含まれる場合，
     * `DeserializeResult::NotEnoughLength`を返す．
     *
     * 短すぎる行は自動で捨てられることはない．
     */
    template <uint8_t MIN_LENGTH>
    class AsyncMinLengthSingleLineBytesDeserializer {
        etl::vector<uint8_t, MIN_LENGTH> buffer_;

        inline bool is_complete() const {
            return is_complete_line(buffer_);
        }

      public:
        template <AsyncReadable Readable>
        nb::Poll<DeserializeResult> deserialize(Readable &readable) {
            while (!buffer_.full()) {
                if (is_complete()) {
                    return DeserializeResult::NotEnoughLength;
                }

                SERDE_DESERIALIZE_OR_RETURN(readable.poll_readable(1));
                buffer_.push_back(readable.read_unchecked());
            }

            return DeserializeResult::Ok;
        }

        inline etl::span<const uint8_t> result() const {
            ASSERT(buffer_.size() == MIN_LENGTH);
            return etl::span{buffer_.begin(), buffer_.size()};
        }

        inline void reset() {
            buffer_.clear();
        }
    };
} // namespace nb::de
