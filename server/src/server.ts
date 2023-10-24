import { NetFacade, AddressType, SinetAddress } from "@core/net";
import { UdpHandler } from "./udp";

// TODO: 環境変数から取得する
const ADDRESS = [0, 1, 2, 3] as const;
const UDP_ADDRESS = new SinetAddress(ADDRESS, 19073);

const main = (): void => {
    console.log("Initializing...");
    const net = new NetFacade();

    const udpHandler = new UdpHandler(UDP_ADDRESS);
    net.addHandler(AddressType.Sinet, udpHandler);
    udpHandler.bind();
};

main();
