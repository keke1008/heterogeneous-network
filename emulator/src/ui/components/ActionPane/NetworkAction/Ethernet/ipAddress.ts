import { z } from "zod";

export const ipAddressSchema = z
    .string()
    .ip({ version: "v4" })
    .transform((str) => str.split(".").map((s) => parseInt(s)))
    .transform((arr) => new Uint8Array(arr));
