import { ObjectMap } from "@core/object";
import { Bridge, IpAddressWithPrefix, IpCommand, NetNs } from "../command/ip";
import { Chain, NftablesCommand, Port, Table } from "../command/nftables";
import { VROUTER_SERVER_LISTEN_PORT } from "./constant";
import { VRouterInterface } from "./interface";
import { sleepMs } from "@core/async";
import { setIpv4Forwarding } from "../command/ipv4Forwarding";

const ROOT_NETNS_NAME = "hg-rt";
const ROOT_TO_DEFAULT_VETH_NAME = "hg-rt-def";
const DEFAULT_TO_ROOT_VETH_NAME = "hg-def-rt";
const ROOT_BRIDGE_NAME = "hg-rt-br";

const vRouterNetNsName = (port: Port) => `hg-vr-${port}`;
const vRouterToDefaultVethName = (port: Port) => `hg-vr-${port}-def`;
const defaultToVRouterVethName = (port: Port) => `hg-def-vr-${port}`;

const NFT_TABLE_NAME = "hg-table";
const NFT_PREROUTING_CHAIN_NAME = "hg-prerouting";
const NFT_POSTROUTING_CHAIN_NAME = "hg-postrouting";

export class VRouterNetwork {
    #port: Port;
    #netNs: NetNs;

    constructor(port: Port, netNs: NetNs) {
        this.#port = port;
        this.#netNs = netNs;
    }

    get port(): Port {
        return this.#port;
    }

    get netNs(): NetNs {
        return this.#netNs;
    }
}

interface RootContext {
    readonly netNs: NetNs;
    readonly bridge: Bridge;
    readonly table: Table;
    readonly prerouting: Chain;
    readonly postrouting: Chain;
}

export class NetNsManager {
    #rootContext: RootContext;
    #vRouterNetworks = new ObjectMap<Port, NetNs, number>((port) => port.toNumber());

    private constructor(rootContext: RootContext) {
        this.#rootContext = rootContext;
    }

    static async create(args: {
        vRouterLocalAddreessRange: IpAddressWithPrefix;
        rootBridgeLocalAddress: IpAddressWithPrefix;
    }): Promise<NetNsManager> {
        await setIpv4Forwarding(true);

        const ip = new IpCommand();
        return await ip.withTransaction(async (tx) => {
            // reset
            const existingNetNs = await tx.getNetNs({ name: ROOT_NETNS_NAME });
            if (existingNetNs !== undefined) {
                console.info(`[netns] deleting existing netns ${ROOT_NETNS_NAME}`);
                await tx.deleteNetNs({ netNs: existingNetNs });

                // netnsに関連したinterfaceは非同期で削除されるので、適当な時間待つ
                await sleepMs(500);
            }

            const netNs = await tx.addNetNs({ name: ROOT_NETNS_NAME });

            const veth = await tx.addVeth({
                connector: { name: ROOT_TO_DEFAULT_VETH_NAME, netNs },
                peer: { name: DEFAULT_TO_ROOT_VETH_NAME },
            });
            await tx.setVethConnectorUp({ vethConnector: veth.connector });
            await tx.setVethConnectorUp({ vethConnector: veth.peer });
            await tx.addRoute({ destination: "default", device: veth.connector });
            await tx.addRoute({ destination: args.vRouterLocalAddreessRange, device: veth.peer });

            const bridge = await tx.addBridge({ name: ROOT_BRIDGE_NAME, netNs });
            await tx.addIpAddress({ device: veth.peer, address: args.rootBridgeLocalAddress });
            await tx.setVethConnectorMaster({ bridge, vethConnector: veth.connector });
            await tx.setBridgeUp({ bridge });

            const nft = new NftablesCommand();
            return await nft.withTransaction(async (tx) => {
                const table = await tx.addTable({ name: NFT_TABLE_NAME, netNs });
                const prerouting = await tx.addNatChain({
                    name: NFT_PREROUTING_CHAIN_NAME,
                    hook: "prerouting",
                    table,
                });
                const postrouting = await tx.addNatChain({
                    name: NFT_POSTROUTING_CHAIN_NAME,
                    hook: "postrouting",
                    table,
                });
                return new NetNsManager({ netNs, bridge, table, prerouting, postrouting });
            });
        });
    }

    async createVRouter(interface_: VRouterInterface): Promise<VRouterNetwork> {
        const { globalPort, localAddress } = interface_;
        if (this.#vRouterNetworks.has(globalPort)) {
            throw new Error(`vRouter for port ${globalPort} already exists`);
        }

        const root = this.#rootContext;
        const ip = new IpCommand();
        return await ip.withTransaction(async (tx) => {
            const vRouterNetNs = await tx.addNetNs({ name: vRouterNetNsName(globalPort) });
            const veth = await tx.addVeth({
                connector: { name: vRouterToDefaultVethName(globalPort), netNs: vRouterNetNs },
                peer: { name: defaultToVRouterVethName(globalPort), netNs: root.netNs },
            });
            await tx.setVethConnectorMaster({ bridge: root.bridge, vethConnector: veth.peer });
            await tx.addIpAddress({ device: veth.connector, address: localAddress });
            await tx.setVethConnectorUp({ vethConnector: veth.connector });
            await tx.setVethConnectorUp({ vethConnector: veth.peer });
            await tx.addRoute({ destination: "default", device: veth.connector });

            const nft = new NftablesCommand();
            await nft.withTransaction(async (tx) => {
                const localPort = Port.schema.parse(VROUTER_SERVER_LISTEN_PORT);
                await tx.addDNatRule({
                    chain: root.prerouting,
                    dport: globalPort,
                    toaddr: localAddress.address,
                    toport: localPort,
                });
                await tx.addSNatRule({
                    chain: root.postrouting,
                    saddr: localAddress.address,
                    sport: localPort,
                    to: globalPort,
                });
            });

            return new VRouterNetwork(globalPort, vRouterNetNs);
        });
    }

    async deleteRoot(): Promise<void> {
        const ip = new IpCommand();
        await ip.withTransaction(async (tx) => {
            await tx.deleteNetNs({ netNs: this.#rootContext.netNs });
        });
    }

    async deleteVRouter(vRouterNetwork: VRouterNetwork): Promise<void> {
        const { port, netNs } = vRouterNetwork;
        const ip = new IpCommand();
        await ip.withTransaction(async (tx) => {
            await tx.deleteNetNs({ netNs });
        });
        this.#vRouterNetworks.delete(port);
    }

    async deleteAllVRouter(): Promise<void> {
        const ip = new IpCommand();
        await ip.withTransaction(async (tx) => {
            for (const netNs of this.#vRouterNetworks.values()) {
                await tx.deleteNetNs({ netNs });
            }
        });
        this.#vRouterNetworks.clear();
    }
}
