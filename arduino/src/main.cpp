#include <board.h>
#include <logger.h>
#include <nb/serial.h>
#include <net/app.h>
#include <undefArduino.h>

util::ArduinoTime time{};
util::ArduinoRand rnd{};

net::App<nb::AsyncReadableWritableSerial<HardwareSerial>> app{time};

memory::Static<nb::AsyncReadableWritableSerial<HardwareSerial>> serial{Serial};
memory::Static<nb::AsyncReadableWritableSerial<HardwareSerial>> serial2{Serial2};
memory::Static<nb::AsyncReadableWritableSerial<HardwareSerial>> serial3{Serial3};

void setup() {
    constexpr int BAUD_RATE = 19200;

    Serial.begin(BAUD_RATE);
    Serial1.begin(BAUD_RATE);
    Serial2.begin(BAUD_RATE);
    Serial3.begin(BAUD_RATE);

    board::setup();
    logger::register_handler(Serial1);

    app.add_serial_port(time, serial);
    app.add_serial_port(time, serial2);
    app.add_serial_port(time, serial3);

    LOG_INFO("Setup complete");
}

void loop() {
    app.execute(time, rnd);
}
