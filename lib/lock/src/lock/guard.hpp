#pragma once

#include "halt.hpp"
#include "mutex.hpp"
#include <etl/utility.h>

namespace lock {

/**
 * リソースへの排他的アクセス権を保持するクラス．
 * このクラスを経由することで，リソースへアクセスできる．
 *
 * このクラスのデストラクタが呼ばれると，排他的アクセス権を返却する．
 *
 * このクラスはコピーができないため，別の変数に移し替えたり，関数の引数として渡す際には
 * `etl::move`の使用が必要な場合がある．
 *
 * @warning `lock::Mutex`がシングルスレッドでのみ正常に動作するため，
 * 割り込み中にアクセスしてはいけない．
 *
 * # Example
 *
 * ```
 * #include "lock.hpp"
 * #include <etl/optional.h>
 * #include <cassert>
 *
 * int main() {
 *     lock::Lock<int> lk(42);
 *     etl::optional<lock::Guard<int>> guard_optional = lk.lock();
 *     lock::Guard<int> guard = guard_optional.value();
 *
 *     int n = *guard + 3; // 45
 * }
 *
 * ```
 */
template <typename T>
class Guard {
    Mutex *mutex_;
    T *value_;

  public:
    explicit constexpr Guard(Mutex *mutex, T *value) : mutex_(mutex), value_(value) {
        if (!mutex_->lock()) {
            halt();
        }
    }

    ~Guard() {
        if (mutex_ != nullptr) {
            mutex_->unlock();
        }
    }

    Guard(const Guard &) = delete;
    Guard &operator=(const Guard &) = delete;

    explicit constexpr Guard(Guard &&other) : mutex_(other.mutex_), value_(other.value_) {
        other.mutex_ = nullptr;
    }

    constexpr Guard &operator=(Guard &&other) {
        if (this != &other) {
            mutex_ = other.mutex_;
            value_ = other.value_;
            other.mutex_ = nullptr;
        }
        return *this;
    }

    constexpr T &operator*() & {
        return *value_;
    }

    constexpr const T &operator*() const & {
        return *value_;
    }

    constexpr T &&operator*() && {
        return etl::move(*value_);
    }

    constexpr T *operator->() {
        return value_;
    }

    constexpr const T *operator->() const {
        return value_;
    }
};

} // namespace lock
