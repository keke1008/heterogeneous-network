#include <doctest.h>

#include <net/frame/service.h>

TEST_CASE("instantiation") {
    net::frame::FrameService<uint32_t> service;
}