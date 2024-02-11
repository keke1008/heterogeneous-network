import { Duration } from "@core/time";
import { NeighborSocketConfig } from "../neighbor";

export const MAX_FRAME_ID_CACHE_SIZE = 8;
export const DEFAULT_REQUEST_TIMEOUT = Duration.fromSeconds(5);
export const CONNECT_TO_ACCESS_POINT_REQUEST_TIMEOUT = Duration.fromSeconds(20);

export const SOCKET_CONFIG: NeighborSocketConfig = {
    doDelay: true,
};
