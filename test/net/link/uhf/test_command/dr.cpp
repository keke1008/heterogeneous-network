#include <doctest.h>
#include <util/doctest_ext.h>

#include <mock/stream.h>
#include <nb/future.h>
#include <net/frame/service.h>
#include <net/link/uhf/command/dr.h>

using namespace net::link;

TEST_CASE("DR") {
    mock::MockReadableWritableStream stream{};
    net::frame::FrameService<Address, 1, 1> frame_service;
    Address peer{uhf::ModemId{0xAB}};
    auto protocol_number = net::frame::ProtocolNumber{001};

    SUBCASE("receive 'abc'") {
        bool discard = false;
        net::link::uhf::DRExecutor executor{discard};

        stream.read_buffer_.write_str("*DR=04\001abc\\RAB\r\n");
        auto poll_opt_frame = executor.poll(frame_service, stream);
        CHECK(poll_opt_frame.is_ready());
        auto opt_frame = etl::move(poll_opt_frame.unwrap());
        CHECK(opt_frame.has_value());

        auto &frame = opt_frame.value();
        CHECK(frame.protocol_number == protocol_number);
        CHECK(frame.peer == peer);
        CHECK(frame.length == 3);
        CHECK(util::as_str(frame.reader.written_buffer()) == "abc");
    }

    SUBCASE("discard frame") {
        bool discard = true;
        net::link::uhf::DRExecutor executor{discard};

        stream.read_buffer_.write_str("*DR=04\001abc\\RAB\r\n");
        auto poll_opt_frame = executor.poll(frame_service, stream);
        CHECK(poll_opt_frame.is_ready());
        CHECK(!poll_opt_frame.unwrap().has_value());
    }
}
