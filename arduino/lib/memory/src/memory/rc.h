#pragma once

#include <etl/utility.h>
#include <stdint.h>

namespace memory {
    template <typename T>
    class CounterCell {
        T value_;
        uint8_t count_ = 1;

      public:
        template <typename... Ts>
        inline constexpr CounterCell(Ts &&...args) : value_{etl::forward<Ts>(args)...} {}

        inline constexpr const T &get() const {
            return value_;
        }

        inline constexpr T &get() {
            return value_;
        }

        inline constexpr void increment() {
            ++count_;
        }

        inline constexpr void decrement() {
            --count_;
        }

        inline constexpr bool is_zero() const {
            return count_ == 0;
        }
    };

    template <typename T>
    CounterCell<T> *as_counter_cell(T *ptr) {
        return reinterpret_cast<CounterCell<T> *>(ptr);
    }

    template <typename T>
    class Rc {
        CounterCell<T> *cell_;

        inline void dettach_cell() {
            if (cell_ == nullptr) {
                return;
            }

            cell_->decrement();
            if (cell_->is_zero()) {
                delete cell_;
                cell_ = nullptr;
            }
        }

      public:
        inline constexpr Rc(CounterCell<T> *cell) : cell_{cell} {
            cell_->increment();
        }

        inline constexpr Rc(const Rc &other) = delete;

        inline constexpr Rc(Rc &&other) : cell_{other.cell_} {
            other.cell_ = nullptr;
        }

        inline constexpr Rc &operator=(const Rc &other) = delete;

        inline constexpr Rc &operator=(Rc &&other) {
            if (cell_ != other.cell_) {
                dettach_cell();
                cell_ = other.cell_;
                other.cell_ = nullptr;
            }
            return *this;
        }

        inline ~Rc() {
            dettach_cell();
        }

        inline constexpr const T &get() const {
            return cell_->get();
        }

        inline constexpr T &get() {
            return cell_->get();
        }

        inline constexpr Rc clone() const {
            return Rc{cell_};
        }
    };
} // namespace memory
