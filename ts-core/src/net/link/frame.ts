import { Address } from "./address";
import { EnumSerdeable } from "@core/serde";

export enum Protocol {
    NoProtocol = 0,
    RoutingNeighbor = 1,
    RoutingReactive = 2,
    Rpc = 3,
    Observer = 4,
    Tunnel = 5,
}

export const PROTOCOL_LENGTH = 1;

export const uint8ToProtocol = (number: number): Protocol => {
    if (!(number in Protocol)) {
        throw new Error(`Unknown protocol number: ${number}`);
    }
    return number;
};

export const protocolToUint8 = (protocol: Protocol): number => {
    return protocol;
};

export const ProtocolSerdeable = new EnumSerdeable<Protocol>(Protocol);

export interface Frame {
    protocol: Protocol;
    remote: Address;
    payload: Uint8Array;
}

export const FRAME_MTU = 254;
