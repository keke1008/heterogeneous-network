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

    class CarrierSenseError {};

    class ModemError {
        etl::variant<ErrorCode, CarrierSenseError> error_;

      public:
        template <typename T>
        ModemError(T &&error) : error_{etl::forward<T>(error)} {}

        template <typename T>
        static ModemError error_code(T &&code) {
            return ModemError{ErrorCode{etl::forward<T>(code)}};
        }

        static ModemError carrier_sense() {
            return ModemError{CarrierSenseError{}};
        }

        inline bool is_error_code() const {
            return etl::holds_alternative<ErrorCode>(error_);
        }

        inline bool is_carrier_sense() const {
            return etl::holds_alternative<CarrierSenseError>(error_);
        }
    };

    template <typename T>
    using ModemResult = nb::Result<T, ModemError>;
} // namespace media::uhf::modem
