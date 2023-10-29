import { NetFacade, AddressType, SinetAddress } from "@core/net";
import { UdpHandler } from "@core/media";

// TODO: 環境変数から取得する
const ADDRESS = [127, 0, 0, 1] as const;
const UDP_ADDRESS = new SinetAddress(ADDRESS, 19073);

const main = async (): Promise<void> => {
    console.log("Initializing...");
    const net = new NetFacade();

    const udpHandler = new UdpHandler(UDP_ADDRESS);
    net.addHandler(AddressType.Sinet, udpHandler);
};

main();
