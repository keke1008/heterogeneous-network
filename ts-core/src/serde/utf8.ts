import { Uint8Serdeable } from "./primitives";
import { TransformSerdeable } from "./transform";
import { VectorSerdeable } from "./vector";

export const Utf8Serdeable = new TransformSerdeable(
    new VectorSerdeable(new Uint8Serdeable()),
    (bytes) => new TextDecoder().decode(new Uint8Array(bytes)),
    (text) => Array.from(new TextEncoder().encode(text)),
);
