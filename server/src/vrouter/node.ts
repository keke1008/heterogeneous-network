import {
    NetFacade,
    NetFacadeBuilder,
    NodeId,
    RoutingService,
    NeighborService,
    Cost,
    AddressType,
    UdpAddress,
} from "@core/net";
import { UdpHandler } from "@core/media/dgram";

export class VRouterNode {
    #routingService: VRouterRoutingService;
    #net: NetFacade;

    constructor(port: number) {
        const builder = new NetFacadeBuilder();
        const neighborService = builder.withDefaultNeighborService();
        this.#routingService = new VRouterRoutingService({ neighborService });
        builder.withRoutingService(this.#routingService);
        this.#net = builder.buildWithDefaults();

        const localAddress = UdpAddress.fromHumanReadableString("0.0.0.0", port).unwrap();
        this.#net.addHandler(AddressType.Udp, new UdpHandler(localAddress));
    }
}
