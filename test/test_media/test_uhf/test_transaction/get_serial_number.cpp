#include <doctest.h>

#include <media/uhf/communicator.h>
#include <media/uhf/get_serial_number.h>
#include <mock/serial.h>
#include <nb/lock.h>
#include <nb/serial.h>
#include <util/u8_literal.h>

using namespace util::u8_literal;

TEST_CASE("GetSerialNumber") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    auto communicator = media::uhf::ModemCommunicator{etl::move(serial)};
    auto lock_communicator = nb::lock::Lock{etl::move(communicator)};
    auto transaction =
        media::uhf::GetSerialNumberTransaction{etl::move(lock_communicator.lock().value())};

    for (auto ch : "*SN=123456789\r\n"_u8it) {
        mock_serial.rx_buffer()->push_back(ch);
    }

    auto result = transaction.execute();
    while (result.is_pending()) {
        result = transaction.execute();
    }

    CHECK(result.unwrap().is_ok());
    auto serial_number = result.unwrap().unwrap_ok();

    int i = 0;
    for (auto ch : "123456789"_u8it) {
        CHECK(serial_number.get()[i] == ch);
        i++;
    }
}
