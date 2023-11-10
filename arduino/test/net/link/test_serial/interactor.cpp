#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <net/frame/service.h>
#include <net/link/serial.h>

using namespace net::link::serial;
using namespace net::link;

TEST_CASE("Interactor") {
    mock::MockReadableWritableStream stream;
    SerialAddress self{012};
    SerialAddress peer{034};

    memory::Static<LinkFrameQueue> queue;
    FrameBroker broker{queue};
    SerialInteractor interactor{stream, broker};

    SUBCASE("address type") {
        auto type = interactor.unicast_supported_address_types();
        CHECK(type.test(AddressType::Serial));

        type = interactor.broadcast_supported_address_types();
        CHECK(type.none());
    }

    SUBCASE("media_info") {
        auto info = interactor.get_media_info();
        CHECK(info.address_type.has_value());
        CHECK(info.address_type.value() == AddressType::Serial);
    }
}
