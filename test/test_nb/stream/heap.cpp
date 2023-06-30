#include <doctest.h>

#include "nb/stream/heap.h"

TEST_CASE("Instantiate heap stream") {
    auto [w, r] = nb::stream::make_heap_stream<uint8_t>(10);
    CHECK_EQ(w.writable_count(), 10);
    CHECK_EQ(r.readable_count(), 0);
    CHECK_FALSE(w.is_closed());
    CHECK_FALSE(r.is_closed());
}
