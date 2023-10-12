#include <doctest.h>

#include <mock/stream.h>
#include <net/link.h>
#include <util/rand.h>

TEST_CASE("instantiation") {
    mock::MockReadableWritableStream stream;
    util::MockTime time{0};
    util::MockRandom rand{0};

    net::link::MediaFacade media{stream, time, rand};
}
