#pragma once

#include <nb/poll.h>
#include <stdint.h>

namespace nb::barrier {
    namespace _private_barrier {
        class Counter {
            uint8_t count_;

          public:
            Counter() = delete;
            Counter(const Counter &) = delete;
            Counter(Counter &&) = default;
            Counter &operator=(const Counter &) = delete;
            Counter &operator=(Counter &&) = default;

            inline constexpr Counter(uint8_t count) : count_{count} {}

            inline constexpr void decrement() {
                --count_;
            }

            nb::Poll<void> poll() {
                return count_ == 0 ? nb::ready() : nb::pending;
            }
        };
    } // namespace _private_barrier

    class Barrier;

    class OwnedBarrier {
        friend class Barrier;

        _private_barrier::Counter counter_;

      public:
        OwnedBarrier() = delete;
        OwnedBarrier(const OwnedBarrier &) = delete;
        OwnedBarrier(OwnedBarrier &&) = delete;
        OwnedBarrier &operator=(const OwnedBarrier &) = delete;
        OwnedBarrier &operator=(OwnedBarrier &&) = delete;

        inline constexpr OwnedBarrier(uint8_t count) : counter_{count} {}

        inline constexpr void decrement() {
            counter_.decrement();
        }

        inline nb::Poll<void> poll() {
            return counter_.poll();
        }
    };

    class Barrier {
        _private_barrier::Counter *counter_;

      public:
        Barrier() = delete;
        Barrier(const Barrier &) = delete;

        Barrier(Barrier &&other) : counter_{other.counter_} {
            other.counter_ = nullptr;
        }

        Barrier &operator=(const Barrier &) = delete;

        Barrier &operator=(Barrier &&other) {
            counter_ = other.counter_;
            counter_ = nullptr;
            return *this;
        }

        inline constexpr Barrier(OwnedBarrier &barrier_) : counter_{&barrier_.counter_} {}

        inline constexpr void decrement() {
            counter_->decrement();
        }

        inline nb::Poll<void> poll() {
            return counter_->poll();
        }
    };
} // namespace nb::barrier
