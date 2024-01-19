#pragma once

#include <nb/lock.h>
#include <nb/serde.h>
#include <stdint.h>
#include <util/span.h>

namespace media::uhf {
    class AsyncUhfResponsePrefixDeserializer {
        bool done_ = false;

      public:
        template <nb::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            if (done_) {
                return nb::de::DeserializeResult::Ok;
            }

            while (readable.poll_readable(1).is_ready()) {
                uint8_t byte = readable.read_unchecked();
                if (byte == '*') {
                    done_ = true;
                    return nb::de::DeserializeResult::Ok;
                }
            }

            return nb::pending;
        }

        inline bool found() const {
            return done_;
        }
    };

    enum class UhfResponseType : uint8_t {
        ER,
        RI,
        SN,
        EI,
        CS,
        DT,
        DR,
        IR,
    };

    class AsyncUhfResponseTypeDeserializer {
        nb::de::Array<nb::de::Bin<uint8_t>, 2> type_{2};
        etl::optional<UhfResponseType> result_;

      public:
        template <nb::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            if (result_.has_value()) {
                return nb::de::DeserializeResult::Ok;
            }

            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(readable));

            const auto &result = type_.result();
            auto str = util::as_str(etl::span<const uint8_t>{result.data(), result.size()});

            if (str == "ER") {
                result_ = UhfResponseType::ER;
            } else if (str == "RI") {
                result_ = UhfResponseType::RI;
            } else if (str == "SN") {
                result_ = UhfResponseType::SN;
            } else if (str == "EI") {
                result_ = UhfResponseType::EI;
            } else if (str == "CS") {
                result_ = UhfResponseType::CS;
            } else if (str == "DT") {
                result_ = UhfResponseType::DT;
            } else if (str == "DR") {
                result_ = UhfResponseType::DR;
            } else if (str == "IR") {
                result_ = UhfResponseType::IR;
            } else {
                return nb::de::DeserializeResult::Invalid;
            }

            return nb::de::DeserializeResult::Ok;
        }

        inline UhfResponseType result() const {
            FASSERT(result_.has_value());
            return *result_;
        }
    };

    class AsyncUhfEqualDeserializer {
        nb::de::Bin<uint8_t> equal_{};

      public:
        template <nb::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(equal_.deserialize(readable));
            return equal_.result() == '=' ? nb::de::DeserializeResult::Ok
                                          : nb::de::DeserializeResult::Invalid;
        }
    };

    class AsyncUhfResponseHeaderDeserializer {
        AsyncUhfResponsePrefixDeserializer prefix_;
        AsyncUhfResponseTypeDeserializer type_;
        AsyncUhfEqualDeserializer equal_;

      public:
        template <nb::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(prefix_.deserialize(readable));
            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(readable));
            return equal_.deserialize(readable);
        }

        inline UhfResponseType result() const {
            return type_.result();
        }

        inline bool is_response_empty() const {
            return !prefix_.found();
        }
    };

    template <nb::AsyncReadable R>
    struct UhfResponse {
        UhfResponseType type;
        nb::LockGuard<etl::reference_wrapper<R>> body;
    };

    template <nb::AsyncReadable R>
    class UhfResponseHeaderReceiver {
        etl::optional<nb::LockGuard<etl::reference_wrapper<R>>> readable_;
        AsyncUhfResponseHeaderDeserializer deserializer_;

      public:
        etl::optional<UhfResponse<R>> execute(nb::Lock<etl::reference_wrapper<R>> &readable) {
            if (!readable_.has_value()) {
                auto poll_r = readable.poll_lock();
                if (poll_r.is_pending()) {
                    return etl::nullopt;
                }
                readable_ = etl::move(poll_r.unwrap());
            }

            auto poll_result = deserializer_.deserialize(readable_.value()->get());
            if (deserializer_.is_response_empty()) {
                readable_.reset();
                return etl::nullopt;
            }
            if (poll_result.is_pending()) {
                return etl::nullopt;
            }

            auto result = poll_result.unwrap() == nb::de::DeserializeResult::Ok
                ? UhfResponse<R>{.type = deserializer_.result(), .body = etl::move(*readable_)}
                : etl::optional<UhfResponse<R>>{};
            readable_.reset();
            deserializer_ = {};
            return etl::move(result);
        }
    };

    enum class UhfHandleResponseResult : uint8_t {
        Invalid,
        Handle,
    };

    template <uint8_t ResponseByteLength>
    class UhfFixedResponseBodyDeserializer {
        // +2 for CR LF
        static constexpr uint8_t Length = ResponseByteLength + 2;
        nb::de::AsyncMaxLengthSingleLineBytesDeserializer<Length> response_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(response_.deserialize(readable));
            return response_.result().size() == Length ? nb::de::DeserializeResult::Ok
                                                       : nb::de::DeserializeResult::Invalid;
        }

        inline const etl::span<const uint8_t> span_result() const {
            return response_.result().subspan(0, ResponseByteLength);
        }

        inline const etl::string_view result() const {
            return util::as_str(span_result());
        }
    };
} // namespace media::uhf
