import { executeCommand } from "./common";
import { P, match } from "ts-pattern";
import { z } from "zod";

export class NetNs {
    #name?: string;

    isDefault(): boolean {
        return this.#name === undefined;
    }

    constructor(args?: { name: string }) {
        this.#name = args?.name;
    }

    static default(): NetNs {
        return new NetNs();
    }

    toString(): string {
        if (this.isDefault()) {
            throw new Error("default netns has no name");
        }
        return this.#name ?? "default";
    }

    equals(other: NetNs): boolean {
        return this.#name === other.#name;
    }

    commandPrefix(): string {
        return this.isDefault() ? "" : `ip netns exec ${this}`;
    }

    static readonly schema: z.ZodType<NetNs, z.ZodTypeDef, unknown> = z
        .string()
        .transform((name) => new NetNs({ name }));
}

export class InterfaceName {
    #name: string;

    constructor(args: { name: string }) {
        this.#name = args.name;
    }

    toString(): string {
        return this.#name;
    }

    static readonly schema: z.ZodType<InterfaceName, z.ZodTypeDef, unknown> = z
        .string()
        .transform((name) => new InterfaceName({ name }));
}

class LinkDevice {
    #name: InterfaceName;
    #netNs: NetNs;

    get netNs(): NetNs {
        return this.#netNs;
    }

    toString(): string {
        return `${this.#name}`;
    }

    constructor(args: { name: InterfaceName; netNs: NetNs }) {
        this.#name = args.name;
        this.#netNs = args.netNs;
    }
}

export class VethConnector extends LinkDevice {
    type: "veth" = "veth" as const;
}

export class Veth {
    #connector: VethConnector;
    #peer: VethConnector;

    get connector(): VethConnector {
        return this.#connector;
    }

    get peer(): VethConnector {
        return this.#peer;
    }

    constructor(args: { connector0: VethConnector; connector1: VethConnector }) {
        this.#connector = args.connector0;
        this.#peer = args.connector1;
    }

    static pair(args: { name: InterfaceName; netNs: NetNs; peerName: InterfaceName; peerNetNs: NetNs }): Veth {
        return new Veth({
            connector0: new VethConnector({ name: args.name, netNs: args.netNs }),
            connector1: new VethConnector({ name: args.peerName, netNs: args.peerNetNs }),
        });
    }

    toString(): string {
        throw new Error("Veth#toString is not implemented");
    }
}

export class Bridge extends LinkDevice {
    type: "bridge" = "bridge" as const;
}

export class IpAddress {
    #address: string;

    private constructor(args: { address: string }) {
        this.#address = args.address;
    }

    toString(): string {
        return this.#address;
    }

    equals(other: IpAddress): boolean {
        return this.#address === other.#address;
    }

    static readonly schema: z.ZodType<IpAddress, z.ZodTypeDef, unknown> = z
        .string()
        .ip({ version: "v4" })
        .transform((address) => new IpAddress({ address }));
}

export class IpAddressWithPrefix {
    #address: IpAddress;
    #prefix: number;

    get address(): IpAddress {
        return this.#address;
    }

    get prefix(): number {
        return this.#prefix;
    }

    constructor(args: { address: IpAddress; prefix: number }) {
        this.#address = args.address;
        this.#prefix = args.prefix;
    }

    static readonly schema: z.ZodType<IpAddressWithPrefix, z.ZodTypeDef, unknown> = z
        .preprocess(
            (val) => String(val).split("/"),
            z.tuple([IpAddress.schema, z.coerce.number().int().min(0).max(32)]),
        )
        .transform(([address, prefix]) => new IpAddressWithPrefix({ address, prefix }));

    toString(): string {
        return `${this.address}/${this.prefix}`;
    }
}

export class Route {
    #destination: IpAddressWithPrefix | "default";
    #gateway?: IpAddress;
    #device: LinkDevice;

    constructor(args: { destination: IpAddressWithPrefix | "default"; gateway?: IpAddress; device: LinkDevice }) {
        this.#destination = args.destination;
        this.#gateway = args.gateway;
        this.#device = args.device;
    }

    get destination(): IpAddressWithPrefix | "default" {
        return this.#destination;
    }

    get gateway(): IpAddress | undefined {
        return this.#gateway;
    }

    get device(): LinkDevice {
        return this.#device;
    }
}

class RawIpCommand {
    async #executeCommand(command: string): Promise<void> {
        await executeCommand(command);
    }

    async #queryCommand<T>(command: string, schema: z.ZodType<T, z.ZodTypeDef, unknown>): Promise<T> {
        const result = await executeCommand(command);
        return schema.parse(JSON.parse(result));
    }

    async addNetNs(args: { name: string }): Promise<NetNs> {
        if (args.name === "") {
            throw new Error("netns name must not be empty");
        }
        const command = `ip netns add ${args.name}`;
        await this.#executeCommand(command);
        return new NetNs({ name: args.name });
    }

    async deleteNetNs(args: { netNs: NetNs }): Promise<void> {
        const command = `ip netns delete ${args.netNs}`;
        return this.#executeCommand(command);
    }

    async getNetNsList(): Promise<NetNs[]> {
        const schema = z
            .array(z.object({ name: z.string(), id: z.string() }))
            .transform((nss) => nss.map((ns) => new NetNs(ns)));
        const command = `ip --json netns list`;
        return this.#queryCommand(command, schema);
    }

    async setLinkDeviceState(device: LinkDevice, state: "up" | "down"): Promise<void> {
        const { netNs = NetNs.default() } = device;
        const command = `${netNs.commandPrefix()} ip link set dev ${device} ${state}`;
        return this.#executeCommand(command);
    }

    async addVeth(args: {
        connector: { name: string; netNs?: NetNs };
        peer: { name: string; netNs?: NetNs };
    }): Promise<Veth> {
        const {
            connector: { name, netNs = NetNs.default() },
            peer: { name: peerName, netNs: peerNetNs = NetNs.default() },
        } = args;
        const netNsOption = netNs.isDefault() ? "" : `netns ${netNs}`;
        const peerNetNsOption = peerNetNs.isDefault() ? "" : `netns ${peerNetNs}`;
        const command = `ip link add ${name} ${netNsOption} type veth peer name ${peerName} ${peerNetNsOption}`;
        await this.#executeCommand(command);
        return Veth.pair({
            name: new InterfaceName({ name }),
            netNs,
            peerName: new InterfaceName({ name: peerName }),
            peerNetNs,
        });
    }

    async deleteLinkDevice(args: { device: LinkDevice | Veth }): Promise<void> {
        const device = args.device instanceof Veth ? args.device.connector : args.device;
        const command = `${device.netNs.commandPrefix()} ip link delete ${device}`;
        return this.#executeCommand(command);
    }

    async getVethConnectorsList(args?: { netNs: NetNs }): Promise<VethConnector[]> {
        const netNs = args?.netNs ?? NetNs.default();
        const schema = z
            .array(z.object({ ifname: InterfaceName.schema, link: z.number() }))
            .transform((connectors) => connectors.map((c) => new VethConnector({ name: c.ifname, netNs })));
        const command = `${netNs.commandPrefix()} ip --json link list type veth`;
        return this.#queryCommand(command, schema);
    }

    async addBridge(args: { name: string; netNs?: NetNs }): Promise<Bridge> {
        const { name, netNs = NetNs.default() } = args;
        const command = `${netNs.commandPrefix()} ip link add ${name} type bridge`;
        await this.#executeCommand(command);
        return new Bridge({ name: new InterfaceName({ name }), netNs: netNs });
    }

    async getBridgeList(args?: { netNs: NetNs }): Promise<Bridge[]> {
        const netNs = args?.netNs ?? NetNs.default();
        const schema = z
            .array(z.object({ ifname: InterfaceName.schema }))
            .transform((bridges) => bridges.map((b) => new Bridge({ name: b.ifname, netNs })));
        const command = `${netNs.commandPrefix()} ip --json link list type bridge`;
        return this.#queryCommand(command, schema);
    }

    async setVethConnectorMaster(args: { bridge: Bridge; vethConnector: VethConnector }): Promise<void> {
        if (!args.bridge.netNs.equals(args.vethConnector.netNs)) {
            throw new Error("netns of bridge and veth connector must be same");
        }

        const { bridge, vethConnector: ctor } = args;
        const command = `${bridge.netNs.commandPrefix()} ip link set ${ctor} master ${bridge}`;
        return this.#executeCommand(command);
    }

    async unsetVethConnectorMaster(args: { vethConnector: VethConnector }): Promise<void> {
        const { vethConnector } = args;
        const command = `${vethConnector.netNs.commandPrefix()} ip link set ${vethConnector} nomaster`;
        return this.#executeCommand(command);
    }

    async addIpAddress(args: { device: LinkDevice; address: IpAddressWithPrefix }): Promise<void> {
        const { device, address } = args;
        const command = `${device.netNs.commandPrefix()} ip address add ${address} dev ${device}`;
        return this.#executeCommand(command);
    }

    async deleteIpAddress(args: { device: LinkDevice; address: IpAddressWithPrefix }): Promise<void> {
        const { device, address } = args;
        const command = `${device.netNs.commandPrefix()} ip address delete ${address} dev ${device}`;
        return this.#executeCommand(command);
    }

    async getIpAddressList(args: { device: LinkDevice }): Promise<IpAddressWithPrefix[]> {
        const { device } = args;
        const addrInfoSchema = z.array(
            z.object({ family: z.string(), local: z.string(), prefixlen: z.coerce.number() }),
        );
        const schema = z
            .tuple([z.object({ addr_info: addrInfoSchema })])
            .transform(([addr]) => addr.addr_info.filter(({ family }) => family === "inet"))
            .pipe(z.array(z.object({ local: IpAddress.schema, prefixlen: z.number() })))
            .transform((addresses) =>
                addresses.map(({ local, prefixlen }) => new IpAddressWithPrefix({ address: local, prefix: prefixlen })),
            );

        const command = `${device.netNs.commandPrefix()} ip --json address show dev ${device}`;
        return this.#queryCommand(command, schema);
    }

    async addRoute(args: {
        device: LinkDevice;
        destination: IpAddressWithPrefix | "default";
        gateway?: IpAddress;
    }): Promise<Route> {
        const { device, destination, gateway } = args;
        const via = gateway ? `via ${gateway}` : "";
        const command = `${device.netNs.commandPrefix()} ip route add ${destination} dev ${device} ${via}`;
        await this.#executeCommand(command);
        return new Route({ device, destination, gateway: gateway });
    }

    async deleteRoute(args: { route: Route }): Promise<void> {
        const { device, destination, gateway } = args.route;
        const via = gateway ? `via ${gateway}` : "";
        const command = `${device.netNs.commandPrefix()} ip route delete ${destination} dev ${device} ${via}`;
        return this.#executeCommand(command);
    }

    async getRouteList(args: { netNs: NetNs }): Promise<Route[]> {
        const schema = z
            .array(z.object({ dst: z.string(), gateway: z.string().optional(), dev: InterfaceName.schema }))
            .transform((routes) =>
                routes.map(({ dst, gateway, dev }) => {
                    return new Route({
                        destination: dst === "default" ? "default" : IpAddressWithPrefix.schema.parse(dst),
                        gateway: gateway ? IpAddress.schema.parse(gateway) : undefined,
                        device: new LinkDevice({ name: dev, netNs: args.netNs }),
                    });
                }),
            );
        const command = `${args.netNs.commandPrefix()} ip --json route list`;
        return this.#queryCommand(command, schema);
    }
}

export class Transaction {
    #ip: RawIpCommand;
    #rollback: (() => Promise<unknown>)[] = [];

    constructor(ip: RawIpCommand) {
        this.#ip = ip;
    }

    async addNetNs(args: { name: string }): Promise<NetNs> {
        const netNs = await this.#ip.addNetNs(args);
        this.#rollback.push(() => this.#ip.deleteNetNs({ netNs }));
        return netNs;
    }

    async deleteNetNs(args: { netNs: NetNs }): Promise<void> {
        await this.#ip.deleteNetNs(args);
        this.#rollback.push(() => this.#ip.addNetNs({ name: args.netNs.toString() }));
    }

    async getNetNs(args: { name: string }): Promise<NetNs | undefined> {
        const nss = await this.#ip.getNetNsList();
        return nss.find((ns) => ns.toString() === args.name);
    }

    async deleteLinkDevice(args: { device: LinkDevice | Veth }): Promise<void> {
        const { device } = args;
        await this.#ip.deleteLinkDevice({ device });
        this.#rollback.push(async () => {
            match(device)
                .with(P.instanceOf(VethConnector), () => {
                    throw new Error("cannot rollback veth connector deletion");
                })
                .with(P.instanceOf(Veth), async (veth) => {
                    await this.#ip.addVeth({
                        connector: { name: veth.connector.toString(), netNs: veth.connector.netNs },
                        peer: { name: veth.peer.toString(), netNs: veth.peer.netNs },
                    });
                })
                .with(P.instanceOf(Bridge), async (bridge) => {
                    await this.#ip.addBridge({ name: bridge.toString(), netNs: bridge.netNs });
                })
                .run();
        });
    }

    async addVeth(args: {
        connector: { name: string; netNs?: NetNs };
        peer: { name: string; netNs?: NetNs };
    }): Promise<Veth> {
        const veth = await this.#ip.addVeth(args);
        this.#rollback.push(() => this.#ip.deleteLinkDevice({ device: veth.connector }));
        return veth;
    }

    async setVethConnectorUp(args: { vethConnector: VethConnector }): Promise<void> {
        await this.#ip.setLinkDeviceState(args.vethConnector, "up");
        this.#rollback.push(() => this.#ip.setLinkDeviceState(args.vethConnector, "down"));
    }

    async getVethConnector(args: { name: string }): Promise<VethConnector | undefined> {
        const veths = await this.#ip.getVethConnectorsList();
        return veths.find((veth) => veth.toString() === args.name);
    }

    async addBridge(args: { name: string; netNs?: NetNs }): Promise<Bridge> {
        const bridge = await this.#ip.addBridge(args);
        this.#rollback.push(() => this.deleteLinkDevice({ device: bridge }));
        return bridge;
    }

    async getBridge(args: { name: string }): Promise<Bridge | undefined> {
        const bridges = await this.#ip.getBridgeList();
        return bridges.find((bridge) => bridge.toString() === args.name);
    }

    async setBridgeUp(args: { bridge: Bridge }): Promise<void> {
        await this.#ip.setLinkDeviceState(args.bridge, "up");
        this.#rollback.push(() => this.#ip.setLinkDeviceState(args.bridge, "down"));
    }

    async setVethConnectorMaster(args: { bridge: Bridge; vethConnector: VethConnector }): Promise<void> {
        await this.#ip.setVethConnectorMaster(args);
        this.#rollback.push(() => this.#ip.unsetVethConnectorMaster(args));
    }

    async unsetVethConnectorMaster(args: { vethConnector: VethConnector }): Promise<void> {
        await this.#ip.unsetVethConnectorMaster(args);
        this.#rollback.push(() => {
            throw new Error("cannot rollback unset veth connector master");
        });
    }

    async addIpAddress(args: { device: LinkDevice; address: IpAddressWithPrefix }): Promise<void> {
        await this.#ip.addIpAddress(args);
        this.#rollback.push(() => this.#ip.deleteIpAddress(args));
    }

    async deleteIpAddress(args: { device: LinkDevice; address: IpAddressWithPrefix }): Promise<void> {
        await this.#ip.deleteIpAddress(args);
        this.#rollback.push(() => this.#ip.addIpAddress(args));
    }

    async getIpAddressList(args: { device: LinkDevice }): Promise<IpAddressWithPrefix[]> {
        return this.#ip.getIpAddressList(args);
    }

    async addRoute(args: {
        device: LinkDevice;
        destination: IpAddressWithPrefix | "default";
        gateway?: IpAddress;
    }): Promise<Route> {
        const route = await this.#ip.addRoute(args);
        this.#rollback.push(() => this.#ip.deleteRoute({ route }));
        return route;
    }

    async deleteRoute(args: { route: Route }): Promise<void> {
        await this.#ip.deleteRoute(args);
        this.#rollback.push(() =>
            this.#ip.addRoute({
                device: args.route.device,
                destination: args.route.destination,
                gateway: args.route.gateway,
            }),
        );
    }

    async getRouteList(args: { netNs: NetNs }): Promise<Route[]> {
        return this.#ip.getRouteList(args);
    }

    async commit(): Promise<void> {
        this.#rollback = [];
    }

    async rollback(): Promise<void> {
        for (const rollback of this.#rollback.reverse()) {
            try {
                await rollback();
            } catch (err) {
                console.warn(`[nft] error during rollback: ${err}`);
            }
        }
        this.#rollback = [];
    }
}

export class IpCommand {
    #raw: RawIpCommand = new RawIpCommand();

    async withTransaction<T>(callback: (tx: Transaction) => Promise<T>): Promise<T> {
        const tx = new Transaction(this.#raw);
        try {
            return await callback(tx);
        } catch (err) {
            console.warn(`[ip] transaction rollback: ${err}`);
            await tx.rollback();
            throw err;
        }
    }
}
