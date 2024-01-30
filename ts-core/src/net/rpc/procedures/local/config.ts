import { Uint8Serdeable } from "@core/serde";
import { BitflagsSerdeable } from "@core/serde/bitflags";

export enum Config {
    enableAutoNeighborDiscovery = 0b00000001,
    enableDynamicCostUpdate = 0b00000010,
    enableFrameDelay = 0b00000100,
}

export const configSerdeable = new BitflagsSerdeable<Config>(Config, new Uint8Serdeable());
