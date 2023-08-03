#include <doctest.h>

#include "etl/optional.h"
#include "media/uhf/carrier_sense.h"

TEST_CASE("CarrierSense") {
    auto mock_serial = mock::MockSerial{};
    auto serial = nb::serial::Serial{mock_serial};

    auto communicator = media::uhf::ModemCommunicator{etl::move(serial)};
    auto lock_communicator = nb::lock::Lock{etl::move(communicator)};
    auto transaction =
        media::uhf::CarrierSenseTransaction{etl::move(lock_communicator.lock().value())};

    for (auto ch : {'*', 'C', 'S', '=', 'E', 'N', '\r', '\n'}) {
        mock_serial.rx_buffer()->push_back(ch);
    }

    auto result = transaction.execute();
    while (result.is_pending()) {
        result = transaction.execute();
    }

    CHECK(result.unwrap().is_ok());
    CHECK(result.unwrap().unwrap_ok());
}
