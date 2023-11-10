#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/link/uhf.h>

using namespace net::link::uhf;
using namespace net::link;

TEST_CASE("UhfInteractor") {
    memory::Static<net::frame::MultiSizeFrameBufferPool<1, 1>> buffer_pool;
    net::frame::FrameService frame_service{buffer_pool};
    mock::MockReadableWritableStream stream{};

    memory::Static<net::link::LinkFrameQueue> link_frame_queue;
    net::link::FrameBroker frame_broker{link_frame_queue};

    UhfInteractor uhf_interactor{stream, frame_broker};

    SUBCASE("address type") {
        auto type = uhf_interactor.unicast_supported_address_types();
        CHECK(type.test(AddressType::UHF));

        type = uhf_interactor.broadcast_supported_address_types();
        CHECK(type.test(AddressType::UHF));
    }

    SUBCASE("media_info") {
        auto info = uhf_interactor.get_media_info();
        CHECK(info.address_type.has_value());
        CHECK(info.address_type.value() == AddressType::UHF);
    }
}
