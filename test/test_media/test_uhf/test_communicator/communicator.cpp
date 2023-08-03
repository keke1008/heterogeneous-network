#include <doctest.h>

#include "media/uhf/communicator.h"

using Serial = nb::serial::Serial<mock::MockSerial>;
using ModemSerialCommand = media::uhf::ModemSerialCommand<Serial>;
using Command = media::uhf::Command<Serial>;
using ModemSerialResponse = media::uhf::ModemSerialResponse<Serial>;
using ResponseName = media::uhf::ResponseName;
using Response = media::uhf::Response<Serial>;

TEST_CASE("ModemSerialCommand") {

    auto mock_serial = mock::MockSerial{};
    auto tx_buffer = mock_serial.tx_buffer();
    auto serial = memory::Owned{nb::serial::Serial{mock_serial}};

    SUBCASE("send command") {
        auto [future, p] = nb::make_future_promise_pair<Command>();
        auto command = ModemSerialCommand{'E', 'I', etl::move(p)};
        CHECK(future.get().is_pending());

        CHECK_FALSE(command.execute(serial));
        CHECK_EQ(tx_buffer->occupied_count(), 3);
        for (auto ch : {'@', 'E', 'I'}) {
            CHECK_EQ(tx_buffer->pop_front(), ch);
        }

        auto poll_writer = future.get();
        CHECK(poll_writer.is_ready());
        auto writer = etl::move(poll_writer.unwrap());
        for (auto ch : {'1', '2'}) {
            writer.body().write(ch);
        }
        CHECK_EQ(tx_buffer->occupied_count(), 2);
        for (auto ch : {'1', '2'}) {
            CHECK_EQ(tx_buffer->pop_front(), ch);
        }

        writer.body().close();
        CHECK(command.execute(serial));
        CHECK_EQ(tx_buffer->occupied_count(), 2);
        for (auto ch : {'\r', '\n'}) {
            CHECK_EQ(tx_buffer->pop_front(), ch);
        }
    }
}

TEST_CASE("ModemSerialResponse") {

    auto mock_serial = mock::MockSerial{};
    auto rx_buffer = mock_serial.rx_buffer();
    auto serial = memory::Owned{nb::serial::Serial{mock_serial}};

    SUBCASE("receive response") {
        auto [future, promise] = nb::make_future_promise_pair<Response>();
        auto response = ModemSerialResponse{etl::move(promise)};
        CHECK(future.get().is_pending());

        for (auto ch : {'*', 'E', 'I'}) {
            rx_buffer->push_back(ch);
        }
        CHECK(response.execute(serial).is_pending());
        CHECK(future.get().is_pending());

        rx_buffer->push_back('=');
        response.execute(serial);
        auto poll_recv = future.get();
        CHECK(poll_recv.is_ready());

        auto recv = etl::move(poll_recv.unwrap());
        auto name = recv.name();
        auto body = etl::move(recv.body());
        CHECK(name == ResponseName::SetEquipmentId);
        CHECK_FALSE(body.is_readable());

        for (auto ch : {'1', '2', '\r', '\n'}) {
            rx_buffer->push_back(ch);
        }
        CHECK(response.execute(serial).is_pending());
        CHECK_EQ(body.readable_count(), 4);
        for (auto ch : {'1', '2'}) {
            CHECK_EQ(body.read(), ch);
        }

        body.close();
        CHECK(response.execute(serial).is_ready());
    }

    SUBCASE("receive invalid terminator") {
        auto [future, promise] = nb::make_future_promise_pair<Response>();
        auto response = ModemSerialResponse{etl::move(promise)};

        for (auto ch : {'*', 'E', 'I', '=', '1', '2', 'R', 'N'}) {
            rx_buffer->push_back(ch);
        }
        response.execute(serial).is_pending();
        auto res = etl::move(future.get().unwrap());
        res.body().read();
        res.body().read();
        res.body().close();

        auto result = response.execute(serial);
        CHECK(result.is_ready());
        CHECK_EQ(result.unwrap().unwrap_err(), media::uhf::InvalidTerminatorError{});
    }
}

TEST_CASE("ModemSerial") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};
    auto modem_serial = media::uhf::ModemCommunicator{etl::move(serial)};

    SUBCASE("send command") {
        auto [command_future, command_promise] = nb::make_future_promise_pair<Command>();
        auto [response_future, response_promise] = nb::make_future_promise_pair<Response>();
        CHECK(modem_serial.send_command(
            'E', 'I', etl::move(command_promise), etl::move(response_promise)
        ));

        modem_serial.execute();
        auto command_poll = command_future.get();
        CHECK(command_poll.is_ready());
        auto command = etl::move(command_poll.unwrap());
        for (auto ch : {'1', '2'}) {
            command.body().write(ch);
        }
        command.body().close();
        modem_serial.execute();

        auto data = mock_serial.tx_buffer();
        for (auto ch : {'@', 'E', 'I', '1', '2', '\r', '\n'}) {
            auto c = data->pop_front();
            CHECK(c.has_value());
            CHECK_EQ(c.value(), ch);
        }

        for (auto ch : {'*', 'E', 'I', '=', '1', '2', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        modem_serial.execute();
        auto response_poll = response_future.get();
        CHECK(response_poll.is_ready());
        auto response = etl::move(response_poll.unwrap());
        CHECK_EQ(response.name(), media::uhf::ResponseName::SetEquipmentId);

        auto response_body = etl::move(response.body());
        for (auto ch : {'1', '2'}) {
            auto c = response_body.read();
            CHECK(c.has_value());
            CHECK_EQ(c.value(), ch);
        }
        response_body.close();
        modem_serial.execute();
    }

    SUBCASE("receive response") {
        for (auto ch : {'*', 'E', 'I', '=', '1', '2', '\r', '\n'}) {
            mock_serial.rx_buffer()->push_back(ch);
        }
        auto response_future = modem_serial.try_receive_response();
        CHECK(response_future.has_value());
        modem_serial.execute();

        auto response_poll = response_future->get();
        CHECK(response_poll.is_ready());
        auto response = etl::move(response_poll.unwrap());

        CHECK(response.name() == media::uhf::ResponseName::SetEquipmentId);

        auto body = etl::move(response.body());
        for (auto ch : {'1', '2'}) {
            auto c = body.read();
            CHECK(c.has_value());
            CHECK_EQ(c.value(), ch);
        }
        body.close();
        modem_serial.execute();
    }
}
