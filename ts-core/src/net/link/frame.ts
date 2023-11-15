import { Address } from "./address";
import { BufferReader } from "../buffer";

export enum Protocol {
    NoProtocol = 0,
    RoutingNeighbor = 1,
    RoutingReactive = 2,
    Rpc = 3,
    Observer = 4,
}

export const numberToProtocol = (number: number): Protocol => {
    if (!(number in Protocol)) {
        throw new Error(`Unknown protocol number: ${number}`);
    }
    return number;
};

export const protocolToNumber = (protocol: Protocol): number => {
    return protocol;
};

export interface Frame {
    protocol: Protocol;
    remote: Address;
    reader: BufferReader;
}

export const FRAME_MTU = 254;
