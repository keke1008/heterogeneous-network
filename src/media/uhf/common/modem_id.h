#pragma once

#include <collection/tiny_buffer.h>
#include <etl/array.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>
#include <util/visitor.h>

namespace media::uhf::common {
    class ModemId {
        collection::TinyBuffer<uint8_t, 2> value_;

      public:
        ModemId() = delete;
        ModemId(const ModemId &) = default;
        ModemId(ModemId &&) = default;
        ModemId &operator=(const ModemId &) = default;
        ModemId &operator=(ModemId &&) = default;

        ModemId(const collection::TinyBuffer<uint8_t, 2> &value) : value_{value} {}

        template <typename... Ts>
        ModemId(Ts... args) : value_{args...} {}

        bool operator==(const ModemId &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const ModemId &other) const {
            return value_ != other.value_;
        }

        const collection::TinyBuffer<uint8_t, 2> &serialize() const {
            return value_;
        }
    };

}; // namespace media::uhf::common
