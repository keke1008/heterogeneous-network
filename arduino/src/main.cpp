#include <app.h>
#include <board.h>
#include <logger.h>
#include <nb/serial.h>
#include <undefArduino.h>

util::ArduinoTime time{};
util::ArduinoRand rnd{};

using RWSerial = nb::AsyncReadableWritableSerial<HardwareSerial>;
using StaticSerial = memory::Static<RWSerial>;
StaticSerial serial{Serial};
StaticSerial serial2{Serial2};
StaticSerial serial3{Serial3};

App<RWSerial> app{time};

void setup() {
    constexpr int BAUD_RATE = 19200;

    Serial.begin(BAUD_RATE);
    Serial1.begin(BAUD_RATE);
    Serial2.begin(BAUD_RATE);
    Serial3.begin(BAUD_RATE);

    board::setup();
    logger::register_handler(Serial1);

    app.add_serial_port(serial, time);
    app.add_serial_port(serial2, time);
    app.add_serial_port(serial3, time);

    LOG_INFO(FLASH_STRING("Setup complete"));
}

void loop() {
    app.execute(time, rnd);
}
