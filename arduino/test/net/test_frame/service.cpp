#include <doctest.h>

#include <net/frame.h>

TEST_CASE("instantiation") {
    memory::Static<net::frame::MultiSizeFrameBufferPool<1, 2>> pool;
    net::frame::FrameService service{pool};
}
