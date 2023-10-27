import { Address } from "./address";
import { BufferReader } from "../buffer";

export enum Protocol {
    NoProtocol = 0,
    RoutingNeighbor = 1,
    Routing = 2,
    Rpc = 3,
    RoutingReactive = 4,
    Notification = 5,
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
    sender: Address;
    reader: BufferReader;
}

export const FRAME_MTU = 254;
