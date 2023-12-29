import { Ok } from "oxide.ts";
import { Deserializer, Serdeable, SerializeResult, Serializer } from "./traits";

export class EmptyDeserializer implements Deserializer<undefined> {
    deserialize() {
        return Ok(undefined);
    }
}

export class EmptySerializer implements Serializer {
    serializedLength(): number {
        return 0;
    }

    serialize(): SerializeResult {
        return Ok(undefined);
    }
}

export class EmptySerdeable implements Serdeable<undefined> {
    serializer(): Serializer {
        return new EmptySerializer();
    }

    deserializer(): Deserializer<undefined> {
        return new EmptyDeserializer();
    }
}
