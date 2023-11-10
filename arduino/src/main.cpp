#include <log.h>
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

    logging::register_handler(Serial);

    app.add_serial_port(time, Serial1);
    app.add_serial_port(time, Serial2);
    app.add_serial_port(time, Serial3);

    LOG_INFO("Setup complete");
}

void loop() {
    app.execute(time, rnd);
}
