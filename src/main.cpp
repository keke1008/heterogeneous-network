#include <Arduino.h>
#include <nb/future.h>
#include <nb/stream.h>
#include <net/link/uhf.h>
#include <util/rand.h>
#include <util/time.h>
#include <util/tuple.h>
#include <util/u8_literal.h>

using namespace net::link;
using namespace util::u8_literal;

constexpr bool is_sender = true;

util::Tuple<uhf::ModemId, uhf::ModemId> resolve_modem_ids() {
    if constexpr (is_sender) {
        return {uhf::ModemId{0xAB}, uhf::ModemId{0xCD}};
    } else {
        return {uhf::ModemId{0xCD}, uhf::ModemId{0xAB}};
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    delay(200);

    util::Time time;
    util::Rand rand;

    nb::stream::SerialStream<HardwareSerial> serial(Serial1);
    uhf::UHFExecutor executor{serial};
    auto [self_id, other_id] = resolve_modem_ids();

    Serial.println("Setting equipment id...");
    auto fut = etl::move(executor.set_equipment_id(self_id).unwrap());
    while (fut.poll().is_pending()) {
        executor.execute(time, rand);
    }
    Serial.println("Equipment id set.");

    delay(1000);

    if (is_sender) {
        constexpr uint8_t LEN = 11;
        nb::stream::FixedReadableBuffer<LEN> content{"Hello World"_u8array};

        nb::Poll<util::Tuple<nb::Future<uhf::CommandWriter>, nb::Future<bool>>> poll = nb::pending;
        while (poll.is_pending()) {
            poll = executor.transmit(other_id, LEN);
        }

        Serial.println("Transmitting...");
        auto [poll_writer, poll_success] = poll.unwrap();
        while (poll_writer.poll().is_pending()) {
            executor.execute(time, rand);
        }

        Serial.println("Transmitting content...");
        auto writer = etl::move(poll_writer.poll().unwrap());
        while (content.read_all_into(writer).is_pending()) {
            executor.execute(time, rand);
        }
        writer.get().close();

        Serial.println("Waiting for success...");
        while (poll_success.poll().is_pending()) {
            executor.execute(time, rand);
        }

        if (poll_success.poll().unwrap()) {
            Serial.println("Success!");
        } else {
            Serial.println("Failure!");
        }
    } else {
        nb::stream::FixedReadableWritableBuffer<255> content;

        Serial.println("Waiting for prepare receive...");
        nb::Poll<nb::Future<uhf::ResponseReader>> poll = nb::pending;
        while (poll.is_pending()) {
            poll = executor.execute(time, rand);
        }

        Serial.println("Waiting for receive...");
        auto fut_reader = etl::move(poll.unwrap());
        while (fut_reader.poll().is_pending()) {
            executor.execute(time, rand);
        }

        Serial.println("Reading content...");
        auto reader = etl::move(fut_reader.poll().unwrap());
        while (content.write_all_from(reader).is_pending()) {
            executor.execute(time, rand);
        }
        reader.get().close();

        Serial.println("Content:");
        while (content.readable_count() > 0) {
            Serial.println(static_cast<char>(content.read()));
        }
    }

    Serial.println("Done!");
}

void loop() {}
