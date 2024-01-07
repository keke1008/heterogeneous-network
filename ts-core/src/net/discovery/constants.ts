import { Duration } from "@core/time";

export const DISCOVERY_FIRST_RESPONSE_TIMEOUT = Duration.fromSeconds(10);
export const DISCOVERY_BETTER_RESPONSE_TIMEOUT_RATE = 1;

export const MAX_DISCOVERY_CACHE_ENTRIES = 4;
export const DISCOVERY_CACHE_EXPIRATION = Duration.fromSeconds(10);
