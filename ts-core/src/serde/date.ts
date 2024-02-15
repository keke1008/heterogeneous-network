import { FixedBytesSerdeable } from "./bytes";
import { Deserializer, Serdeable, Serializer } from "./traits";
import { TransformSerdeable } from "./transform";

export class DateSerdeable implements Serdeable<Date> {
    #serdeable: Serdeable<Date>;

    constructor() {
        this.#serdeable = new TransformSerdeable(
            new FixedBytesSerdeable(8),
            (bytes) => {
                const view = new DataView(bytes.buffer);
                const timestamp = view.getFloat64(0, true);
                return new Date(timestamp);
            },
            (date) => {
                const timestamp = date.getTime();
                const buffer = new ArrayBuffer(8);
                const view = new DataView(buffer);
                view.setFloat64(0, timestamp, true);
                return new Uint8Array(buffer);
            },
        );
    }

    deserializer(): Deserializer<Date> {
        return this.#serdeable.deserializer();
    }

    serializer(value: Date): Serializer {
        return this.#serdeable.serializer(value);
    }
}
