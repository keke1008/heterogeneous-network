// #include <doctest.h>
//
// #include <media/uhf/modem.h>
//
// TEST_CASE("UHFId") {
//     SUBCASE("instantiation") {
//         media::uhf::modem::ModemId id{{1, 2}};
//         CHECK_EQ(id.serialize(), collection::TinyBuffer<uint8_t, 2>{1, 2});
//     }
//
//     SUBCASE("equality") {
//         media::uhf::modem::ModemId id1{{1, 2}};
//         media::uhf::modem::ModemId id2{{1, 2}};
//         CHECK_EQ(id1, id2);
//     }
// }
//
// TEST_CASE("CommandName") {
//     auto name = media::uhf::modem::CommandName::CarrierSense;
//     CHECK_EQ(
//         media::uhf::modem::serialize_command_name(name),
//         collection::TinyBuffer<uint8_t, 2>{'C', 'S'}
//     );
// }
//
// TEST_CASE("Command") {
//     nb::stream::StreamReaderDelegate<media::uhf::modem::Command<nb::stream::TinyByteReader<2>>>
//         command{media::uhf::modem::CommandName::SetEquipmentId, 'A', 'B'};
//     nb::stream::TinyByteWriter<7> writer;
//     nb::stream::pipe(command, writer);
//
//     CHECK(command.is_reader_closed());
//     CHECK(writer.is_writer_closed());
//
//     auto serialized = writer.poll();
//     CHECK(serialized.is_ready());
//     auto expected = collection::TinyBuffer<uint8_t, 7>{'@', 'E', 'I', 'A', 'B', '\r', '\n'};
//     CHECK_EQ(serialized.unwrap(), expected);
// }
//
// TEST_CASE("ResponseName") {
//     auto name = media::uhf::modem::deserialize_response_name({'C', 'S'});
//     CHECK(name.has_value());
//     CHECK_EQ(name.value(), media::uhf::modem::ResponseName::CarrierSense);
// }
//
// TEST_CASE("ResponseHeader") {
//     SUBCASE("known response name") {
//         nb::stream::StreamWriterDelegate<media::uhf::modem::ResponseHeader> header;
//         auto input = nb::stream::TinyByteReader<8>{'*', 'E', 'I', '=', 'A', 'B', '\r', '\n'};
//         nb::stream::pipe(input, header);
//
//         CHECK_EQ(input.readable_count(), 4);
//         CHECK(header.is_writer_closed());
//
//         auto actual = header.get_writer().poll();
//         CHECK(actual.is_ready());
//         CHECK(actual.unwrap().is_ok());
//         CHECK_EQ(actual.unwrap().unwrap_ok(), media::uhf::modem::ResponseName::EquipmentId);
//     }
//
//     SUBCASE("unknown response name") {
//         nb::stream::StreamWriterDelegate<media::uhf::modem::ResponseHeader> header;
//         auto input = nb::stream::TinyByteReader<8>{'*', 'U', 'N', '=', 'A', 'B', '\r', '\n'};
//         nb::stream::pipe(input, header);
//
//         CHECK_EQ(input.readable_count(), 4);
//         CHECK(header.is_writer_closed());
//
//         auto actual = header.get_writer().poll();
//         CHECK(actual.is_ready());
//         CHECK(actual.unwrap().is_err());
//     }
// }
//
// TEST_CASE("Response") {
//     nb::stream::StreamWriterDelegate<media::uhf::modem::Response<nb::stream::TinyByteWriter<2>>>
//         response;
//     auto input = nb::stream::TinyByteReader<4>{'A', 'B', '\r', '\n'};
//     nb::stream::pipe(input, response);
//
//     CHECK(input.is_reader_closed());
//     CHECK(response.is_writer_closed());
//     auto actual = response.get_writer().get_body().poll();
//     CHECK(actual.is_ready());
//     CHECK_EQ(actual.unwrap(), collection::TinyBuffer<uint8_t, 2>{'A', 'B'});
// }
