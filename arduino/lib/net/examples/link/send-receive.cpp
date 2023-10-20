// Serial1を使用して通信を行う．

#include <undefArduino.h>

#include <nb/serial.h>
#include <net/link.h>
#include <util/rand.h>
#include <util/time.h>

inline constexpr bool is_sender = true;

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial.println("Begin");

    util::ArduinoTime time{};
    util::ArduinoRand rand{};
    nb::serial::SerialStream serial{Serial1};
    net::link::MediaFacade media{serial, time};

    while (media.wait_for_media_detection(time).is_pending()) {}
    Serial.println("Media detected");

    auto address = net::link::Address{net::link::SerialAddress{0x01}};
    uint8_t length = 0x05;

    delay(2000);

    if (is_sender) {
        Serial.println("Requesting transmission");
        auto future = media.send_frame(address, length);
        while (future.poll().is_pending()) {
            media.execute(time, rand);
        }

        Serial.println("Sending frame");
        auto frame = future.poll().unwrap();
        etl::array<uint8_t, 5> content{'H', 'E', 'L', 'L', 'O'};
        nb::stream::FixedReadableBuffer<5> buffer{content};

        Serial.println("Waiting for transmission");
        while (buffer.read_all_into(frame.get().body).is_pending()) {
            media.execute(time, rand);
        }
        frame.get().body.close();

        Serial.println("Transmission complete");
    } else {
        Serial.println("Waiting for frame");
        auto poll = media.execute(time, rand);
        while (poll.is_pending()) {
            poll = media.execute(time, rand);
        }

        Serial.println("Receiving frame");
        auto frame = etl::move(poll.unwrap());
        nb::stream::FixedWritableBuffer<5> content{};
        while (content.write_all_from(frame.body).is_pending()) {
            media.execute(time, rand);
        }
        frame.body.close();
        Serial.println("Received frame");

        Serial.print("Content: ");
        for (char c : content.written_bytes()) {
            Serial.print(c);
        }
        Serial.println();
        Serial.println("Reception complete");
    }
}

void loop() {}
