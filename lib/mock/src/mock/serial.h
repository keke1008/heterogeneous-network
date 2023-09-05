#pragma once

#include <etl/array.h>
#include <etl/utility.h>
#include <memory/circular_buffer.h>
#include <memory/shared.h>
#include <stdint.h>

namespace mock {
    class MockSerial {
        memory::Shared<memory::CircularBuffer<uint8_t[64]>> tx_buffer_ =
            memory::CircularBuffer<etl::array<uint8_t, 64>>::make_stack<64>();
        memory::Shared<memory::CircularBuffer<uint8_t[64]>> rx_buffer_ =
            memory::CircularBuffer<etl::array<uint8_t, 64>>::make_stack<64>();

      public:
        MockSerial() = default;
        MockSerial(const MockSerial &) = default;
        MockSerial &operator=(const MockSerial &) = default;

        MockSerial(MockSerial &&other)
            : tx_buffer_{other.tx_buffer_},
              rx_buffer_{other.rx_buffer_} {}

        MockSerial &operator=(MockSerial &&other) {
            tx_buffer_ = other.tx_buffer_;
            rx_buffer_ = other.rx_buffer_;
            return *this;
        }

        MockSerial(
            memory::Shared<memory::CircularBuffer<uint8_t[64]>> &tx_buffer,
            memory::Shared<memory::CircularBuffer<uint8_t[64]>> &rx_buffer
        )
            : tx_buffer_{tx_buffer},
              rx_buffer_{rx_buffer} {}

        operator bool() {
            return true;
        }

        void begin(unsigned long baud){};
        void begin(unsigned long, uint8_t){};
        void end(){};

        int available() {
            return rx_buffer_->occupied_count();
        }

        int peek() {
            auto value = rx_buffer_->peek_front();
            return value ? *value : -1;
        }

        int read() {
            auto value = rx_buffer_->pop_front();
            return value ? *value : -1;
        }

        int availableForWrite() {
            return tx_buffer_->vacant_count();
        }

        size_t write(uint8_t byte) {
            return tx_buffer_->push_back(byte) ? 1 : 0;
        }

        memory::Shared<memory::CircularBuffer<uint8_t[64]>> tx_buffer() {
            return tx_buffer_;
        }

        memory::Shared<memory::CircularBuffer<uint8_t[64]>> rx_buffer() {
            return rx_buffer_;
        }
    };

    etl::pair<MockSerial, MockSerial> make_connected_mock_serials();
} // namespace mock
