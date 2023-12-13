import { NetFacadeBuilder, AddressType } from "@core/net";
import { UdpHandler } from "@core/media/dgram";
import { VROUTER_SERVER_LISTEN_PORT } from "./constant";

const main = () => {
    const net = new NetFacadeBuilder().buildWithDefaults();
    net.addHandler(AddressType.Udp, new UdpHandler(VROUTER_SERVER_LISTEN_PORT));
};

main();
