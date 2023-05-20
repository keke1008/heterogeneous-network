#pragma once

#include "lock/guard.hpp"
#include "lock/mutex.hpp"
#include <etl/optional.h>
#include <etl/utility.h>

namespace lock {

/**
 * リソースへの排他的アクセスを提供するクラス．
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
 *
 *     {
 *         // アクセス権を取得する
 *         etl::optional<lock::Guard<int>> guard = lk.lock();
 *         assert(guard.has_value());
 *
 *         // 既に変数`guard`によってアクセス権を奪われているため，取得に失敗する
 *         etl::optional<lock::Guard<int>> guard2 = lk.lock();
 *         assert(!guard2.has_value());
 *
 *     // ここで変数`guard`のスコープが終わるため，`Guard<T>`のデストラクタが呼ばれる
 *     }
 *
 *     // デストラクタにより返却されたため，再度アクセス権を取得できる．
 *     etl::optional<lock::Guard<int>> guard3 = lk.lock();
 *     assert(guard3.has_value());
 *
 * }
 * ```
 */
template <typename T>
class Lock {
    Mutex mutex_;
    T value_;

  public:
    /**
     * リソースを受け取り，`Lock`を構築する．
     *
     * # Example
     *
     * ```
     * struct Human {
     *     int age;
     *     String name;
     * };
     *
     * Human human = { 42, "YAMADA TARO" };
     * lock::Lock<Human> human_lock = lock::Lock(human);
     * ```
     */
    explicit constexpr Lock(T &&value) : value_(etl::move(value)) {}

    /**
     * リソースへの排他的アクセス権を獲得する．
     *
     * 獲得に成功すると，戻り値の`etl::optional`は`lock::Guard<T>`を保持する．
     * 失敗すると，戻り値は空となる．
     *
     * `lock::Guard<T>`のデストラクタが呼ばれると．アクセス権を返却する．
     *
     * # Example
     * ```
     * struct Human {
     *     int age;
     *     String name;
     * };
     *
     * Human human = { 42, "YAMADA TARO" };
     * lock::Lock<Human> human_lock = lock::Lock(etl::move(human));
     *
     * etl::optional<lock::Guard<int>> guard = human_lock.lock();
     * guard.age = 18;
     * ```
     */
    constexpr etl::optional<Guard<T>> lock() & {
        if (mutex_.is_locked()) {
            return etl::nullopt;
        } else {
            return etl::move(Guard(&mutex_, &value_));
        }
    }

    /**
     * 排他的アクセス権を取得可能かを表す．
     *
     * `true`の場合，すでに別のプログラムがアクセス権を持っており，返却されるまで待つ必要がある．
     * `false`の場合は，すぐにアクセス権を取得できる状態である．
     *
     * # Example
     *
     * ```
     * lock::Lock<int> int_lock = lock::Lock(42);
     * assert(!int_lock.is_locked());
     *
     * {
     *     etl::optional<lock::Guard<int>> guard = int_lock.lock();
     *     assert(int_lock.is_locked());
     * }
     *
     * assert(!int_lock.is_locked());
     * ```
     *
     */
    constexpr bool is_locked() const & {
        return mutex_.is_locked();
    }
};

} // namespace lock
