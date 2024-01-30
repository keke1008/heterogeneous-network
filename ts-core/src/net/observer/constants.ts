import { Duration } from "@core/time";
import { NeighborSocketConfig } from "../neighbor";

export const MAX_FRAME_ID_CACHE_SIZE = 8;
export const SOCKET_CONFIG: NeighborSocketConfig = {
    doDelay: false,
};

export const DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS = 1 * 60 * 1000; // 1 minute
export const NOTIFY_NODE_SUBSCRIPTION_INTERVAL_MS = DELETE_NODE_SUBSCRIPTION_TIMEOUT_MS / 2; // 30 seconds
export const NODE_NOTIFICATION_THROTTLE = Duration.fromSeconds(0.5);

export const DELETE_NETWORK_SUBSCRIPTION_TIMEOUT_MS = 1 * 60 * 1000; // 1 minute
export const NOTIFY_NETWORK_SUBSCRIPTION_INTERVAL_MS = DELETE_NETWORK_SUBSCRIPTION_TIMEOUT_MS / 2; // 30 seconds
export const NETWORK_NOTIFICATION_THROTTLE = Duration.fromSeconds(1);
