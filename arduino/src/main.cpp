#include <net/app.h>
#include <undefArduino.h>
#include <util/exception.h>

util::ArduinoTime time{};
util::ArduinoRand rnd{};

net::App app{time};

void setup() {
    util::set_error_handler();

    app.add_serial_port(time, Serial1);
    app.add_serial_port(time, Serial2);
    app.add_serial_port(time, Serial3);
}

void loop() {
    app.execute(time, rnd);
}
