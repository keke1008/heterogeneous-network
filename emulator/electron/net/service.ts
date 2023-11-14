import { Address, AddressType, Cost, LinkSendError, NetFacade, SerialAddress, UdpAddress } from "@core/net";
import { UdpHandler } from "@core/media/dgram";
import { LinkStateService, ModifyResult } from "./linkState";
import { SerialHandler, SerialPortPath } from "./serial";
import { Result } from "oxide.ts";

export class NetService {
    #net: NetFacade = new NetFacade();
    #linkState: LinkStateService = new LinkStateService(this.#net);
    #serialHandler?: SerialHandler;

    begin(args: { selfUdpAddress: UdpAddress; selfSerialAddress: SerialAddress }): void {
        const udpHandler = new UdpHandler(args.selfUdpAddress);
        this.#net.addHandler(AddressType.Udp, udpHandler);

        this.#serialHandler = new SerialHandler(args.selfSerialAddress);
        this.#net.addHandler(AddressType.Serial, this.#serialHandler);
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

    onGraphModified(onGraphModified: (result: ModifyResult) => void): () => void {
        if (this.#linkState === undefined) {
            throw new Error("LinkStateService is not initialized");
        }
        return this.#linkState.onGraphModified(onGraphModified);
    }

    end(): void {
        this.#linkState?.onDispose();
    }
}
