import { Address, AddressType, Cost, LinkSendError, NetFacade, NodeId, SerialAddress, UdpAddress } from "@core/net";
import { UdpHandler } from "@core/media/dgram";
import { LinkStateService, StateUpdate } from "./linkState";
import { SerialHandler, SerialPortPath } from "./serial";
import { Result } from "oxide.ts";

export class NetService {
    #net: NetFacade;
    #linkState: LinkStateService;
    #serialHandler: SerialHandler;

    constructor(args: { localUdpAddress: UdpAddress; localSerialAddress: SerialAddress; localCost?: Cost }) {
        const localId = NodeId.fromAddress(args.localSerialAddress);
        this.#net = new NetFacade(localId, args.localCost ?? new Cost(0));

        this.#linkState = new LinkStateService(this.#net);

        this.#serialHandler = new SerialHandler(args.localSerialAddress);
        this.#net.addHandler(AddressType.Serial, this.#serialHandler);

        const udpHandler = new UdpHandler(args.localUdpAddress);
        this.#net.addHandler(AddressType.Udp, udpHandler);
    }

    async getUnconnectedSerialPorts(): Promise<SerialPortPath[]> {
        if (this.#serialHandler === undefined) {
            throw new Error("SerialHandler is not initialized");
        }
        return this.#serialHandler.getUnconnectedPorts();
    }

    async connectSerial(args: {
        portPath: SerialPortPath;
        address: SerialAddress;
        cost: Cost;
    }): Promise<Result<void, Error | LinkSendError>> {
        if (this.#serialHandler === undefined) {
            throw new Error("SerialHandler is not initialized");
        }
        const result = await this.#serialHandler.connect(args.portPath, args.address);
        if (result.isErr()) {
            return result;
        }

        return this.#net.routing().requestHello(new Address(args.address), args.cost);
    }

    connectUdp(args: { address: UdpAddress; cost: Cost }): Result<void, LinkSendError> {
        return this.#net.routing().requestHello(new Address(args.address), args.cost);
    }

    onNetStateUpdate(onStateUpdate: (update: StateUpdate) => void): void {
        this.#linkState.onStateUpdate(onStateUpdate);
    }

    end(): void {
        this.#net.dispose();
    }
}
