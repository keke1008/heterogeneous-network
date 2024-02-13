import { Duration } from "@core/time";

export const RETRY_INTERVAL = Duration.fromSeconds(1);
export const ACK_TIMEOUT = Duration.fromSeconds(10);
export const RETRY_COUNT = 5;
