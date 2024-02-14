import { Uint8Serdeable } from "./primitives";
import { Deserializer, Serdeable, Serializer } from "./traits";
import { TransformSerdeable } from "./transform";
import { VectorSerdeable } from "./vector";

export class Utf8Serdeable implements Serdeable<string> {
    #serdeable: Serdeable<string>;

    constructor(lengthSerdeable: Serdeable<number> = new Uint8Serdeable()) {
        this.#serdeable = new TransformSerdeable(
            new VectorSerdeable(new Uint8Serdeable(), lengthSerdeable),
            (bytes) => new TextDecoder().decode(new Uint8Array(bytes)),
            (text) => Array.from(new TextEncoder().encode(text)),
        );
    }

    deserializer(): Deserializer<string> {
        return this.#serdeable.deserializer();
    }

    serializer(value: string): Serializer {
        return this.#serdeable.serializer(value);
    }
}
