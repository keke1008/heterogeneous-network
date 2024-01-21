#include <app.h>
#include <board.h>
#include <logger.h>
#include <nb/serial.h>
#include <undefArduino.h>

util::ArduinoTime time{};
util::ArduinoRand rnd{};

using RWSerial = nb::AsyncReadableWritableSerial<HardwareSerial>;
App<RWSerial> app{time};

memory::Static<RWSerial> serial{Serial};
memory::Static<RWSerial> serial2{Serial2};
memory::Static<RWSerial> serial3{Serial3};

memory::Static<media::SerialPortMediaPort<RWSerial>> serial_port0{serial, app.frame_queue(), time};
memory::Static<media::SerialPortMediaPort<RWSerial>> serial_port1{serial2, app.frame_queue(), time};
memory::Static<media::SerialPortMediaPort<RWSerial>> serial_port2{serial3, app.frame_queue(), time};
memory::Static<media::EthernetShieldMediaPort> ethernet_port{app.frame_queue(), time};

void setup() {
    constexpr int BAUD_RATE = 19200;

    Serial.begin(BAUD_RATE);
    Serial1.begin(BAUD_RATE);
    Serial2.begin(BAUD_RATE);
    Serial3.begin(BAUD_RATE);

    board::setup();
    logger::register_handler(Serial1);

    LOG_INFO(FLASH_STRING("Setup start"));

    ethernet_port->initialize(rnd);

    app.register_port(serial_port0);
    app.register_port(serial_port1);
    app.register_port(serial_port2);
    app.register_port(ethernet_port);

    LOG_INFO(FLASH_STRING("Setup complete"));
}

void loop() {
    app.execute(time, rnd);
}
