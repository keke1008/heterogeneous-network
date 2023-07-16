#pragma once

#include <collection/tiny_buffer.h>
#include <etl/variant.h>
#include <nb/result.h>

namespace media::uhf::executor {
    class CarrierSenseTimeoutError {};

    class ModemErrorCodeError {
        collection::TinyBuffer<uint8_t, 2> code_;

      public:
        ModemErrorCodeError(collection::TinyBuffer<uint8_t, 2> code) : code_{code} {}

        const collection::TinyBuffer<uint8_t, 2> &error_code() const {
            return code_;
        }
    };

    class PacketDroppedError {};

    using ExecutionError =
        etl::variant<CarrierSenseTimeoutError, ModemErrorCodeError, PacketDroppedError>;

    template <typename T>
    using ExecutionResult = nb::Result<T, ExecutionError>;
} // namespace media::uhf::executor
