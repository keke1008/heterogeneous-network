#include "etl/memory.h"
#include <etl/utility.h>
#include <stddef.h>

namespace memory {
    using etl::declval, etl::is_same;

    template <typename T, typename E>
    using is_buffer = is_same<etl::decay_t<decltype(declval<T>()[declval<size_t>()])>, E>;

    template <typename T, typename E>
    inline constexpr bool is_buffer_v = is_buffer<T, E>::value;
}; // namespace memory
