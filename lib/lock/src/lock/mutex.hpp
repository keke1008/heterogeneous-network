#pragma once

namespace lock {

/**
 * 未使用か使用中かを表す型．要するにただの`bool`型の値．
 *
 * @see https://wa3.i-3-i.info/word13360.html
 * @warning スレッドセーフないため，割り込み中などにアクセスしてはいけない
 */
class Mutex {
    bool locked_;

  public:
    /**
     * 使用可能状態の`Mutex`を作成する
     *
     * # Example
     *
     * ```
     * lock::Mutex mu = lock::Mutex();
     * assert(!mu.is_locked());
     * ```
     */
    explicit constexpr Mutex() : locked_(false) {}

    /**
     * Mutexをロックし，使用中にする．
     *
     * 呼び出す前から使用中の状態であった場合，ロックは失敗し，`false`を返す．
     *
     * 呼び出す前から未使用の状態であった場合，ロックは成功し，`true`を返す．
     *
     * # Example
     *
     * ```
     * lock::Mutex mu = lock::Mutex();
     * assert(mu.lock());
     * assert(!mu.lock());
     * ```
     */
    constexpr bool lock() & {
        if (locked_) {
            return false;
        }
        locked_ = true;
        return true;
    }

    /**
     * Mutexのロックを解除し，未使用の状態に戻す．
     *
     * # Example
     *
     * ```
     * lock::Mutex mu = lock::Mutex();
     * mu.lock();
     * mu.unlock();
     * ```
     */
    constexpr void unlock() & {
        locked_ = false;
    }

    /**
     * Mutexが使用中かを返す．
     *
     * # Example
     *
     * ```
     * lock::Mutex mu = lock::Mutex();
     * assert(!mu.is_locked());
     *
     * mu.lock();
     * assert(mu.is_locked());
     * ```
     */
    constexpr bool is_locked() const & {
        return locked_;
    }
};

} // namespace lock
