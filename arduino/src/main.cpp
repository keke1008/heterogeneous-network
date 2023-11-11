#include <logger.h>
#include <net/app.h>
#include <undefArduino.h>

util::ArduinoTime time{};
util::ArduinoRand rnd{};

net::App app{time};

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial2.begin(9600);
    Serial3.begin(9600);

    logger::register_handler(Serial1);

    memory::Static<nb::stream::SerialStream<HardwareSerial>> serial{Serial};
    memory::Static<nb::stream::SerialStream<HardwareSerial>> serial2{Serial2};
    memory::Static<nb::stream::SerialStream<HardwareSerial>> serial3{Serial3};

    app.add_serial_port(time, serial);
    app.add_serial_port(time, serial2);
    app.add_serial_port(time, serial3);

    LOG_INFO("Setup complete");
}

void loop() {
    app.execute(time, rnd);
}
