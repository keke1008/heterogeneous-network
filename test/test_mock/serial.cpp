#include <doctest.h>

#include <mock/serial.h>

TEST_CASE("Instantate MockSerial") {
    mock::MockSerial mock_serial;
    CHECK_EQ(mock_serial.available(), 64);
    CHECK_EQ(mock_serial.availableForWrite(), 64);
}

TEST_CASE("Write once to MockSerial") {
    mock::MockSerial mock_serial;
    CHECK_EQ(mock_serial.write(0x01), 1);
    CHECK_EQ(mock_serial.availableForWrite(), 63);
    CHECK_EQ(mock_serial.available(), 64);
    CHECK_EQ(mock_serial.tx_buffer()->pop_back(), 0x01);
}

TEST_CASE("Read once from MockSerial") {
    mock::MockSerial mock_serial;
    mock_serial.rx_buffer()->push_back(0x01);
    CHECK_EQ(mock_serial.available(), 63);
    CHECK_EQ(mock_serial.availableForWrite(), 64);
    CHECK_EQ(mock_serial.peek(), 0x01);
    CHECK_EQ(mock_serial.read(), 0x01);
    CHECK_EQ(mock_serial.available(), 64);
}
