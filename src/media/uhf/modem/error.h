#pragma once

#include <collection/tiny_buffer.h>
#include <etl/array.h>
#include <etl/utility.h>
#include <etl/variant.h>
#include <nb/result.h>

namespace media::uhf::modem {
    class ErrorCode {
        etl::array<uint8_t, 2> code_;

      public:
        ErrorCode(const etl::array<uint8_t, 2> &code) : code_{code} {}

        const etl::array<uint8_t, 2> &get() const {
            return code_;
        }
    };

    class InvalidCharacterError {};

    class InvalidResponseName {
        collection::TinyBuffer<uint8_t, 2> name_;

      public:
        InvalidResponseName(const collection::TinyBuffer<uint8_t, 2> &name) : name_{name} {}

        const collection::TinyBuffer<uint8_t, 2> &get() const {
            return name_;
        }
    };

    class CarrierSenseError {};

    class ModemError {
        etl::variant<ErrorCode, InvalidCharacterError, InvalidResponseName, CarrierSenseError>
            error_;

      public:
        template <typename T>
        ModemError(T &&error) : error_{etl::forward<T>(error)} {}

        template <typename T>
        static ModemError error_code(T &&code) {
            return ModemError{ErrorCode{etl::forward<T>(code)}};
        }

        static ModemError invalid_character() {
            return ModemError{InvalidCharacterError{}};
        }

        template <typename T>
        static ModemError invalid_response_name(T &&name) {
            return ModemError{InvalidResponseName{etl::forward<T>(name)}};
        }

        static ModemError carrier_sense() {
            return ModemError{CarrierSenseError{}};
        }

        inline bool is_error_code() const {
            return etl::holds_alternative<ErrorCode>(error_);
        }

        inline bool is_invalid_character() const {
            return etl::holds_alternative<InvalidCharacterError>(error_);
        }

        inline bool is_invalid_response_name() const {
            return etl::holds_alternative<InvalidResponseName>(error_);
        }

        inline bool is_carrier_sense() const {
            return etl::holds_alternative<CarrierSenseError>(error_);
        }
    };

    template <typename T>
    using ModemResult = nb::Result<T, ModemError>;
} // namespace media::uhf::modem
