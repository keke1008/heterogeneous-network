import { Duration } from "@core/time";

export interface ColorPalette {
    nodeDefault: string;
    nodeSelected: string;
    nodeReceived: string;
    nodeText: string;
    link: string;
    text: string;
}

export const RECEIVED_HIGHLIGHT_TIMEOUT = Duration.fromSeconds(2);
