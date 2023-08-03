#pragma once

#include <etl/variant.h>
#include <nb/result.h>

namespace media::uhf {
    enum class ResponseName {
        CarrierSense,
        GetSerialNumber,
        SetEquipmentId,
        DataTransmission,
        DataReceiving,
        Error,
        Information,
        Other,
    };

    class UnhandlableResponseNameError {
        ResponseName response_name_;

      public:
        explicit UnhandlableResponseNameError(ResponseName response_name)
            : response_name_{response_name} {}

        ResponseName response_name() const {
            return response_name_;
        }

        bool operator==(const UnhandlableResponseNameError &other) const {
            return response_name_ == other.response_name_;
        }

        bool operator!=(const UnhandlableResponseNameError &other) const {
            return !(*this == other);
        }
    };

    class InvalidTerminatorError {
      public:
        bool operator==(const InvalidTerminatorError &) const {
            return true;
        }

        bool operator!=(const InvalidTerminatorError &) const {
            return false;
        }
    };

    class UHFError {
        etl::variant<UnhandlableResponseNameError, InvalidTerminatorError> error_;

      public:
        explicit UHFError(UnhandlableResponseNameError error) : error_{error} {}

        explicit UHFError(InvalidTerminatorError error) : error_{error} {}

        bool is_unhandlable_response_name_error() const {
            return etl::holds_alternative<UnhandlableResponseNameError>(error_);
        }

        bool is_invalid_terminator_error() const {
            return etl::holds_alternative<InvalidTerminatorError>(error_);
        }

        UnhandlableResponseNameError &unhandlable_response_name_error() {
            return etl::get<UnhandlableResponseNameError>(error_);
        }

        InvalidTerminatorError &invalid_terminator_error() {
            return etl::get<InvalidTerminatorError>(error_);
        }

        const etl::variant<UnhandlableResponseNameError, InvalidTerminatorError> &error() const {
            return error_;
        }
    };

    template <typename T>
    using UHFResult = nb::Result<T, UHFError>;
} // namespace media::uhf
