#pragma once

#include <etl/utility.h>

namespace util {
    /**
     * 型Tをコピーできないようにする
     *
     * `etl::variant`など，何故かコピー不可な型をコピーできてしまう型があるので，それを防ぐために使う
     */
    template <typename T>
    class NonCopyable {
        T value_;

      public:
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable(NonCopyable &&) = default;
        NonCopyable &operator=(const NonCopyable &) = delete;
        NonCopyable &operator=(NonCopyable &&) = default;

        explicit NonCopyable(T &&value) : value_{etl::move(value)} {}

        template <typename... Args>
        explicit NonCopyable(Args &&...args) : value_{etl::forward<Args>(args)...} {}

        T &get() {
            return value_;
        }

        const T &get() const {
            return value_;
        }
    };
} // namespace util
