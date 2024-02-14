import { Uint64Serdeable } from "./primitives";
import { Deserializer, Serdeable, Serializer } from "./traits";
import { TransformSerdeable } from "./transform";

export class DateSerdeable implements Serdeable<Date> {
    #serdeable: Serdeable<Date>;

    constructor() {
        this.#serdeable = new TransformSerdeable(
            new Uint64Serdeable(),
            (timestamp) => new Date(timestamp),
            (date) => date.getTime(),
        );
    }

    deserializer(): Deserializer<Date> {
        return this.#serdeable.deserializer();
    }

    serializer(value: Date): Serializer {
        return this.#serdeable.serializer(value);
    }
}
