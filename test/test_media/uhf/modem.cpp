#include <doctest.h>

#include <media/uhf/modem.h>
#include <mock/serial.h>
#include <nb/lock.h>
//
// TEST_CASE("UHFId") {
//     SUBCASE("Instantiate") {
//         media::uhf::modem::ModemId id{{1, 2}};
//         CHECK_EQ(id.serialize(), collection::TinyBuffer<uint8_t, 2>{1, 2});
//     }
//
//     SUBCASE("Equal") {
//         media::uhf::modem::ModemId id1{{1, 2}};
//         media::uhf::modem::ModemId id2{{1, 2}};
//         CHECK_EQ(id1, id2);
//     }
// }
//
//
// TEST_CASE("FixedCommand") {
//     SUBCASE("Instantiate") {
//         media::uhf::modem::FixedCommand<'E', 'I', 2> command{'0', '1'};
//         CHECK(command.is_readable());
//         CHECK_EQ(command.readable_count(), 7);
//         CHECK_FALSE(command.is_closed());
//     }
//
//     SUBCASE("Read") {
//         media::uhf::modem::FixedCommand<'E', 'I', 2> command{'0', '2'};
//         for (auto ch : {'@', 'E', 'I', '0', '2', '\r', '\n'}) {
//             auto value = command.read();
//             CHECK(value.has_value());
//             CHECK_EQ(*value, ch);
//         }
//         CHECK_FALSE(command.read().has_value());
//         CHECK_EQ(command.readable_count(), 0);
//         CHECK(command.is_closed());
//     }
// }
//
// TEST_CASE("FixedResponse") {
//     SUBCASE("Instantiate") {
//         media::uhf::modem::FixedResponse<2> response;
//         CHECK(response.is_writable());
//         CHECK_EQ(response.writable_count(), 8);
//         CHECK_FALSE(response.is_closed());
//     }
//
//     SUBCASE("Write") {
//         media::uhf::modem::FixedResponse<2> response;
//         for (auto ch : {'*', 'E', 'I', '=', '0', '2', '\r', '\n'}) {
//             CHECK(response.write(ch));
//         }
//         CHECK_FALSE(response.write('X'));
//         CHECK_EQ(response.writable_count(), 0);
//         CHECK(response.is_closed());
//
//         CHECK_EQ(response.prefix(), etl::array<uint8_t, 1>{'*'});
//         CHECK_EQ(response.command_name(), etl::array<uint8_t, 2>{'E', 'I'});
//         CHECK_EQ(response.equal(), etl::array<uint8_t, 1>{'='});
//         auto result = response.get_result();
//         CHECK(result.is_ok());
//         CHECK_EQ(result.get_ok(), etl::array<uint8_t, 2>{'0', '2'});
//     }
// }
//
// TEST_CASE("FixedQuery") {
//     media::uhf::modem::Query<
//         media::uhf::modem::FixedCommand<'E', 'I', 2>, media::uhf::modem::FixedResponse<2>>
//         query{{'0', '2'}, {}};
//
//     REQUIRE(query.is_writable());
//     REQUIRE_EQ(query.writable_count(), 8);
//     REQUIRE(query.is_readable());
//     REQUIRE_EQ(query.readable_count(), 7);
//     REQUIRE_FALSE(query.is_closed());
//     REQUIRE_FALSE(query.response());
//
//     SUBCASE("Query") {
//         for (auto ch : {'@', 'E', 'I', '0', '2', '\r', '\n'}) {
//             auto value = query.read();
//             CHECK(value.has_value());
//             CHECK_EQ(*value, ch);
//         }
//         CHECK_FALSE(query.read().has_value());
//         CHECK_EQ(query.readable_count(), 0);
//
//         for (auto ch : {'*', 'E', 'I', '=', '0', '2', '\r', '\n'}) {
//             CHECK(query.write(ch));
//         }
//         CHECK_FALSE(query.write('X'));
//         CHECK_EQ(query.writable_count(), 0);
//         CHECK(query.is_closed());
//     }
// }
//
// TEST_CASE("TransmissionCommand") {
//     SUBCASE("Instantiate") {
//         uint8_t length = 10;
//         auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
//         media::uhf::modem::PacketTransmissionCommand command{length, etl::move(rx)};
//         CHECK(command.is_readable());
//         CHECK_FALSE(command.is_closed());
//     }
//
//     SUBCASE("Read") {
//         uint8_t length = 10;
//         auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
//         for (auto i = 0; i < length; ++i) {
//             tx.write(i);
//         }
//         media::uhf::modem::PacketTransmissionCommand command{length, etl::move(rx)};
//
//         for (uint8_t ch : {'@', 'D', 'T', '0', 'A'}) {
//             auto value = command.read();
//             CHECK(value.has_value());
//             CHECK_EQ(*value, ch);
//         }
//         for (auto i = 0; i < length; ++i) {
//             auto value = command.read();
//             CHECK(value.has_value());
//             CHECK_EQ(*value, i);
//         }
//         for (uint8_t ch : {'\r', '\n'}) {
//             auto value = command.read();
//             CHECK(value.has_value());
//             CHECK_EQ(*value, ch);
//         }
//         CHECK_FALSE(command.read().has_value());
//         CHECK(command.is_closed());
//     }
// }
//
// TEST_CASE("TransmissionQuery") {
//     SUBCASE("Instantiate") {
//         uint8_t length = 10;
//         auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
//         media::uhf::modem::PacketTransmissionQuery query{length, etl::move(rx)};
//         CHECK(query.is_writable());
//         CHECK(query.is_readable());
//         CHECK_FALSE(query.is_closed());
//         auto result = query.get_result();
//         CHECK_FALSE(result.is_pending());
//     }
//
//     SUBCASE("Query") {
//         uint8_t length = 3;
//         auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
//         for (auto ch : {'A', 'B', 'C'}) {
//             tx.write(ch);
//         }
//         media::uhf::modem::PacketTransmissionQuery query{length, etl::move(rx)};
//
//         for (uint8_t ch : {'@', 'D', 'T', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
//             auto value = query.read();
//             CHECK(value.has_value());
//             CHECK_EQ(*value, ch);
//         }
//
//         for (uint8_t ch : {'*', 'D', 'T', '=', '0', '3', '\r', '\n'}) {
//             CHECK(query.write(ch));
//         }
//
//         auto result = query.get_result();
//         CHECK(result.is_ok());
//     }
// }
//
// TEST_CASE("ReceivingResponse") {
//     SUBCASE("Instantiate") {
//         uint8_t length = 10;
//         media::uhf::modem::PacketReceivingResponse response;
//         CHECK(response.is_writable());
//         CHECK_FALSE(response.is_closed());
//     }
//
//     SUBCASE("Response") {
//         media::uhf::modem::PacketReceivingResponse response;
//         for (auto ch : {'*', 'D', 'T', '=', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
//             CHECK(response.write(ch));
//         }
//         CHECK_FALSE(response.write('D'));
//         CHECK(response.is_closed());
//
//         auto result = response.get_result();
//         CHECK(result.is_ok());
//         auto data = result.get_ok().make_reader();
//         for (auto ch : {'A', 'B', 'C'}) {
//             auto value = data.read();
//             CHECK(value.has_value());
//             CHECK_EQ(*value, ch);
//         }
//     }
// }
//
// TEST_CASE("ReceivingQuery") {
//     SUBCASE("Receive") {
//         media::uhf::modem::PacketReceivingQuery query;
//         CHECK(query.is_writable());
//         CHECK_FALSE(query.is_closed());
//         CHECK(query.get_result().is_pending());
//
//         for (auto ch : {'*', 'D', 'T', '=', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
//             CHECK(query.write(ch));
//         }
//         CHECK_FALSE(query.write('D'));
//         CHECK(query.is_closed());
//         CHECK(query.get_result().is_ok());
//     }
// }
