import { Duration } from "@core/time";

export const SEND_HELLO_ITERVAL = Duration.fromSeconds(10);
export const NEIGHBOR_EXPIRATION_TIMEOUT = SEND_HELLO_ITERVAL.multiply(4);
