#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/service.h>

using namespace net::link;

TEST_CASE("instantiation") {
    mock::MockReadableWritableStream stream;
    util::MockTime time{0};
    util::MockRandom rand{0};

    etl::array<MediaFacade, 1> media{MediaFacade{stream, time, rand}};
    net::frame::FrameService<Address> frame_service{};
    MediaService<net::frame::FrameService<Address>, MediaFacade, 1> service{frame_service, media};

    service.wait_for_media_detection();
    service.get_addresses();
}
