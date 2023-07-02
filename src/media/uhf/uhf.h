#pragma once

#include "./modem.h"
#include "media/packet.h"
#include <etl/optional.h>
#include <memory/circular_buffer.h>

namespace media {
    constexpr inline size_t EXECUTOR_BUFFER_SIZE = 2;

    template <typename T>
    class UHF {
        uhf::modem::ModemCommunicator<T> communicator_;
        etl::optional<uhf::modem::Executor<T>> executor_;
        memory::CircularBuffer<media::UHFPacket[3]> packet_buffer_{
            memory::CircularBuffer<media::UHFPacket[3]>::make_stack<3>()};

      public:
        using StreamWriterItem = UHFPacket;
        using StreamReaderItem = UHFPacket;

        UHF(const UHF &) = delete;
        UHF(UHF &&) = default;
        UHF &operator=(const UHF &) = delete;
        UHF &operator=(UHF &&) = default;

        UHF(nb::serial::Serial<T> &&serial) : communicator_{etl::move(serial)} {}

        bool is_writable() const {
            return !packet_buffer_.is_full();
        }

        size_t writable_count() const {
            return !packet_buffer_.vacant_count();
        }

        bool write(UHFPacket &&packet) {}
    };
} // namespace media
