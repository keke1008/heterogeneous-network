import { Address, AddressType, Cost, NetFacade, NodeId, UdpAddress } from "@core/net";
import { UdpHandler } from "@core/media";
import { LinkStateService, ModifyResult } from "./linkState";

export class NetService {
    #net: NetFacade;
    #linkState?: LinkStateService;

    constructor() {
        this.#net = new NetFacade();
    }

    begin(args: { selfAddress: string; selfPort: string }): void {
        const port = parseInt(args.selfPort);
        const addr = UdpAddress.fromHumanReadableString(args.selfAddress, port);
        const handler = new UdpHandler(addr);
        this.#net.addHandler(AddressType.Sinet, handler);
        this.#linkState = new LinkStateService(this.#net, NodeId.fromAddress(addr));
    }

    onGraphModified(onGraphModified: (result: ModifyResult) => void): () => void {
        if (this.#linkState === undefined) {
            throw new Error("LinkStateService is not initialized");
        }
        return this.#linkState.onGraphModified(onGraphModified);
    }

    connect(args: { address: string; port: string; cost: number }): void {
        const port_ = parseInt(args.port);
        const addr = UdpAddress.fromHumanReadableString(args.address, port_);
        this.#net.routing().requestHello(new Address(addr), new Cost(args.cost));
    }

    end(): void {
        this.#linkState?.onDispose();
    }
}
