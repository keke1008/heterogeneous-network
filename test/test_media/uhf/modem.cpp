#include <doctest.h>

#include <media/uhf/modem.h>
#include <mock/serial.h>
#include <nb/lock.h>

TEST_CASE("UHFId") {
    SUBCASE("Instantiate") {
        media::uhf::modem::ModemId id{{1, 2}};
        CHECK_EQ(id.value(), etl::array<uint8_t, 2>{1, 2});
    }

    SUBCASE("Equal") {
        media::uhf::modem::ModemId id1{{1, 2}};
        media::uhf::modem::ModemId id2{{1, 2}};
        CHECK_EQ(id1, id2);
    }
}

TEST_CASE("FixedCommand") {
    SUBCASE("Instantiate") {
        media::uhf::modem::FixedCommand<2> command{{'E', 'I'}, {'0', '1'}};
        CHECK(command.is_readable());
        CHECK_EQ(command.readable_count(), 7);
        CHECK_FALSE(command.is_closed());
    }

    SUBCASE("Read") {
        media::uhf::modem::FixedCommand<2> command{{'E', 'I'}, {'0', '2'}};
        for (auto ch : {'@', 'E', 'I', '0', '2', '\r', '\n'}) {
            auto value = command.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(command.read().has_value());
        CHECK_EQ(command.readable_count(), 0);
        CHECK(command.is_closed());
    }
}

TEST_CASE("FixedResponse") {
    SUBCASE("Instantiate") {
        media::uhf::modem::FixedResponse<2> response;
        CHECK(response.is_writable());
        CHECK_EQ(response.writable_count(), 8);
        CHECK_FALSE(response.is_closed());
    }

    SUBCASE("Write") {
        media::uhf::modem::FixedResponse<2> response;
        for (auto ch : {'*', 'E', 'I', '=', '0', '2', '\r', '\n'}) {
            CHECK(response.write(ch));
        }
        CHECK_FALSE(response.write('X'));
        CHECK_EQ(response.writable_count(), 0);
        CHECK(response.is_closed());

        CHECK_EQ(response.prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response.command_name(), etl::array<uint8_t, 2>{'E', 'I'});
        CHECK_EQ(response.equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response.data(), etl::array<uint8_t, 2>{'0', '2'});
        CHECK_EQ(response.terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }
}

TEST_CASE("FixedQuery") {
    SUBCASE("Instantiate") {
        media::uhf::modem::FixedQuery<2, 2> query{{{'E', 'I'}, {'0', '2'}}};
        CHECK(query.is_writable());
        CHECK_EQ(query.writable_count(), 8);
        CHECK(query.is_readable());
        CHECK_EQ(query.readable_count(), 7);
        CHECK_FALSE(query.is_closed());
        CHECK_FALSE(query.response());
    }

    SUBCASE("Query") {
        media::uhf::modem::FixedQuery<2, 2> query{{{'E', 'I'}, {'0', '2'}}};

        for (auto ch : {'@', 'E', 'I', '0', '2', '\r', '\n'}) {
            auto value = query.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(query.read().has_value());
        CHECK_EQ(query.readable_count(), 0);

        for (auto ch : {'*', 'E', 'I', '=', '0', '2', '\r', '\n'}) {
            CHECK(query.write(ch));
        }
        CHECK_FALSE(query.write('X'));
        CHECK_EQ(query.writable_count(), 0);
        CHECK(query.is_closed());
    }
}

TEST_CASE("FixedExecutor") {
    SUBCASE("Instantiate") {
        mock::MockSerial mock_serial;
        nb::serial::Serial<mock::MockSerial> serial(mock_serial);
        nb::lock::Lock lock(etl::move(serial));
        media::uhf::modem::FixedCommand<2> command{{'E', 'I'}, {'0', '2'}};
        media::uhf::modem::FixedExecutor<mock::MockSerial, 2, 2> executor{
            etl::move(*lock.lock()), etl::move(command)};
        auto response = executor.execute();
        CHECK_FALSE(response.has_value());
    }

    SUBCASE("Execute") {
        mock::MockSerial mock_serial;
        nb::serial::Serial<mock::MockSerial> serial(mock_serial);
        nb::lock::Lock lock(etl::move(serial));
        media::uhf::modem::FixedCommand<2> command{{'E', 'I'}, {'0', '2'}};
        media::uhf::modem::FixedExecutor<mock::MockSerial, 2, 2> executor{
            etl::move(*lock.lock()), etl::move(command)};

        CHECK_FALSE(executor.execute().has_value());
        for (auto ch : {'@', 'E', 'I', '0', '2', '\r', '\n'}) {
            auto value = mock_serial.tx_buffer()->pop_front();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }

        for (auto ch : {'*', 'E', 'I', '=', '0', '2', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor.execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'E', 'I'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->data(), etl::array<uint8_t, 2>{'0', '2'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }
}

TEST_CASE("TransmissionCommand") {
    SUBCASE("Instantiate") {
        uint8_t length = 10;
        auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
        media::uhf::modem::TransmissionCommand command{length, etl::move(rx)};
        CHECK(command.is_readable());
        CHECK_FALSE(command.is_closed());
    }

    SUBCASE("Read") {
        uint8_t length = 10;
        auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
        for (auto i = 0; i < length; ++i) {
            tx.write(i);
        }
        media::uhf::modem::TransmissionCommand command{length, etl::move(rx)};

        for (uint8_t ch : {'@', 'D', 'T', '0', 'A'}) {
            auto value = command.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        for (auto i = 0; i < length; ++i) {
            auto value = command.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, i);
        }
        for (uint8_t ch : {'\r', '\n'}) {
            auto value = command.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(command.read().has_value());
        CHECK(command.is_closed());
    }
}

TEST_CASE("TransmissionQuery") {
    SUBCASE("Instantiate") {
        uint8_t length = 10;
        auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
        media::uhf::modem::TransmissionQuery query{{length, etl::move(rx)}};
        CHECK(query.is_writable());
        CHECK(query.is_readable());
        CHECK_FALSE(query.is_closed());
        CHECK_FALSE(query.response());
    }

    SUBCASE("Query") {
        uint8_t length = 3;
        auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
        for (auto ch : {'A', 'B', 'C'}) {
            tx.write(ch);
        }
        media::uhf::modem::TransmissionQuery query{{length, etl::move(rx)}};

        for (uint8_t ch : {'@', 'D', 'T', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
            auto value = query.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }

        for (uint8_t ch : {'*', 'D', 'T', '=', '0', '3', '\r', '\n'}) {
            CHECK(query.write(ch));
        }

        auto response = query.response();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'D', 'T'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->data(), etl::array<uint8_t, 2>{'0', '3'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }
}

TEST_CASE("TransmissionExecutor") {
    SUBCASE("Instantiate") {
        uint8_t length = 3;
        auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
        mock::MockSerial mock_serial;
        nb::serial::Serial<mock::MockSerial> serial(mock_serial);
        nb::lock::Lock lock(etl::move(serial));
        media::uhf::modem::TransmissionExecutor executor{
            etl::move(*lock.lock()), {length, etl::move(rx)}};
        CHECK_FALSE(executor.execute().has_value());
    }

    SUBCASE("Execute") {
        uint8_t length = 3;
        auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
        for (auto ch : {'A', 'B', 'C'}) {
            tx.write(ch);
        }
        mock::MockSerial mock_serial;
        nb::serial::Serial<mock::MockSerial> serial(mock_serial);
        nb::lock::Lock lock(etl::move(serial));
        media::uhf::modem::TransmissionExecutor executor{
            etl::move(*lock.lock()), {length, etl::move(rx)}};

        CHECK_FALSE(executor.execute().has_value());
        for (uint8_t ch : {'@', 'D', 'T', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
            auto value = mock_serial.tx_buffer()->pop_front();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }

        for (uint8_t ch : {'*', 'D', 'T', '=', '0', '3', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor.execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'D', 'T'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->data(), etl::array<uint8_t, 2>{'0', '3'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }
}

TEST_CASE("ReceivingResponse") {
    SUBCASE("Instantiate") {
        uint8_t length = 10;
        media::uhf::modem::ReceivingResponse response;
        CHECK(response.is_writable());
        CHECK_FALSE(response.is_closed());
    }

    SUBCASE("Response") {
        media::uhf::modem::ReceivingResponse response;
        for (auto ch : {'*', 'D', 'T', '=', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
            CHECK(response.write(ch));
        }
        CHECK_FALSE(response.write('D'));
        CHECK(response.is_closed());

        CHECK_EQ(response.prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response.command_name(), etl::array<uint8_t, 2>{'D', 'T'});
        CHECK_EQ(response.equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response.length(), etl::array<uint8_t, 2>{'0', '3'});
        CHECK_EQ(response.terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
        auto data = response.data().make_reader();
        for (auto ch : {'A', 'B', 'C'}) {
            auto value = data.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
    }
}

TEST_CASE("ReceivingExecutor") {
    SUBCASE("Instantiate") {
        mock::MockSerial mock_serial;
        nb::serial::Serial<mock::MockSerial> serial(mock_serial);
        nb::lock::Lock lock(etl::move(serial));
        media::uhf::modem::ReceivingExecutor executor{etl::move(*lock.lock())};
        CHECK_FALSE(executor.execute().has_value());
    }

    SUBCASE("Execute") {
        mock::MockSerial mock_serial;
        nb::serial::Serial<mock::MockSerial> serial(mock_serial);
        nb::lock::Lock lock(etl::move(serial));
        media::uhf::modem::ReceivingExecutor executor{etl::move(*lock.lock())};

        for (uint8_t ch : {'*', 'D', 'T', '=', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor.execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'D', 'T'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
        auto data = response->data().make_reader();
        for (auto ch : {'A', 'B', 'C'}) {
            auto value = data.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
    }
}

TEST_CASE("ModemCommunicator") {
    mock::MockSerial mock_serial;
    nb::serial::Serial<mock::MockSerial> serial(mock_serial);
    media::uhf::modem::ModemCommunicator communicator{etl::move(serial)};

    SUBCASE("Cannot execute more than two queries at the same time") {
        auto executor1 = communicator.get_serial_number();
        CHECK(executor1.has_value());
        auto executor2 = communicator.get_serial_number();
        CHECK_FALSE(executor2.has_value());
        auto executor3 = communicator.get_serial_number();
        CHECK_FALSE(executor3.has_value());
    }

    SUBCASE("get_serial_number") {
        auto executor = communicator.get_serial_number();
        CHECK(executor.has_value());

        CHECK_FALSE(executor->execute().has_value());
        for (uint8_t ch : {'@', 'S', 'N', '\r', '\n'}) {
            auto value = mock_serial.tx_buffer()->pop_front();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(executor->execute().has_value());

        for (uint8_t ch :
             {'*', 'S', 'N', '=', '1', '2', '3', 'A', 'B', 'C', 'D', 'E', 'F', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor->execute();
        CHECK(response.has_value());

        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'S', 'N'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(
            response->data(), etl::array<uint8_t, 9>{'1', '2', '3', 'A', 'B', 'C', 'D', 'E', 'F'}
        );
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }

    SUBCASE("set_equipment_id") {
        media::uhf::modem::ModemId id{'1', 'A'};
        auto executor = communicator.set_equipment_id(id);
        CHECK(executor.has_value());

        CHECK_FALSE(executor->execute().has_value());
        for (uint8_t ch : {'@', 'E', 'I', '1', 'A', '\r', '\n'}) {
            auto value = mock_serial.tx_buffer()->pop_front();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(executor->execute().has_value());

        for (uint8_t ch : {'*', 'E', 'I', '=', '1', 'A', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor->execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'E', 'I'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->data(), etl::array<uint8_t, 2>{'1', 'A'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }

    SUBCASE("set_destination_id") {
        media::uhf::modem::ModemId id{'A', '1'};
        auto executor = communicator.set_destination_id(id);
        CHECK(executor.has_value());

        CHECK_FALSE(executor->execute().has_value());
        for (uint8_t ch : {'@', 'D', 'I', 'A', '1', '\r', '\n'}) {
            auto value = mock_serial.tx_buffer()->pop_front();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(executor->execute().has_value());

        for (uint8_t ch : {'*', 'D', 'I', '=', 'A', '1', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor->execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'D', 'I'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->data(), etl::array<uint8_t, 2>{'A', '1'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }

    SUBCASE("carrier_sence") {
        auto executor = communicator.carrier_sense();
        CHECK(executor.has_value());

        CHECK_FALSE(executor->execute().has_value());
        for (uint8_t ch : {'@', 'C', 'S', '\r', '\n'}) {
            auto value = mock_serial.tx_buffer()->pop_front();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(executor->execute().has_value());

        for (uint8_t ch : {'*', 'C', 'S', '=', 'E', 'N', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor->execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'C', 'S'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->data(), etl::array<uint8_t, 2>{'E', 'N'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }

    SUBCASE("transmit_packet") {
        uint8_t length = 0x03;
        auto [tx, rx] = nb::stream::make_heap_stream<uint8_t>(length);
        for (uint8_t ch : {'A', 'B', 'C'}) {
            tx.write(ch);
        }
        auto executor = communicator.transmit_packet(length, etl::move(rx));

        CHECK_FALSE(executor->execute().has_value());
        for (uint8_t ch : {'@', 'D', 'T', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
            auto value = mock_serial.tx_buffer()->pop_front();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
        CHECK_FALSE(executor->execute().has_value());

        for (uint8_t ch : {'*', 'D', 'T', '=', '0', '3', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response = executor->execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'D', 'T'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->data(), etl::array<uint8_t, 2>{'0', '3'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
    }

    SUBCASE("receive_packet empty") {
        auto executor = communicator.receive_packet();
        CHECK_FALSE(executor.has_value());
    }

    SUBCASE("receive_packet") {
        for (uint8_t ch : {'*', 'D', 'R', '=', '0', '3', 'A', 'B', 'C', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }

        auto executor = communicator.receive_packet();
        CHECK(executor.has_value());

        auto response = executor->execute();
        CHECK(response.has_value());
        CHECK_EQ(response->prefix(), etl::array<uint8_t, 1>{'*'});
        CHECK_EQ(response->command_name(), etl::array<uint8_t, 2>{'D', 'R'});
        CHECK_EQ(response->equal(), etl::array<uint8_t, 1>{'='});
        CHECK_EQ(response->length(), etl::array<uint8_t, 2>{'0', '3'});
        CHECK_EQ(response->terminator(), etl::array<uint8_t, 2>{'\r', '\n'});
        auto data = response->data().make_reader();
        for (uint8_t ch : {'A', 'B', 'C'}) {
            auto value = data.read();
            CHECK(value.has_value());
            CHECK_EQ(*value, ch);
        }
    }
}
