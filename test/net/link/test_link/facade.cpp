#include <doctest.h>

#include <mock/stream.h>
#include <net/link.h>

TEST_CASE("instantiation") {
    mock::MockReadableWritableStream stream;
    util::MockTime time{0};

    net::link::MediaFacade media{stream, time};
}
