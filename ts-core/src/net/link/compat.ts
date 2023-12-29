import { TransformSerdeable, Uint8Serdeable } from "@core/serde";
import * as z from "zod";

export class MediaPortNumber {
    #value: number;

    constructor(value: number) {
        this.#value = value;
    }

    static serdeable = new TransformSerdeable(
        new Uint8Serdeable(),
        (value) => new MediaPortNumber(value),
        (value) => value.#value,
    );

    static schema = z
        .union([z.number(), z.string().min(1)])
        .pipe(z.coerce.number().min(0).max(255))
        .transform((value) => new MediaPortNumber(value));
}
