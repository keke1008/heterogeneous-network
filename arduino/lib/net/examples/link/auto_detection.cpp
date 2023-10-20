#include <undefArduino.h>

#include <nb/serial.h>
#include <net/link.h>
#include <util/time.h>

void setup() {
    Serial.begin(9600);
    Serial1.begin(115200);
    Serial.println("Begin");

    util::ArduinoTime time{};
    nb::serial::SerialStream serial{Serial1};
    net::link::MediaFacade media{serial, time};

    while (media.wait_for_media_detection(time).is_pending()) {}
    Serial.println("Media detected");
}

void loop() {}
