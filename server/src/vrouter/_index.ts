import * as path from "node:path";
import { fileURLToPath } from "node:url";
import { spawn, ChildProcess } from "node:child_process";
import { NetNsManager } from "./netns";
import { VRouterInterfaceStore } from "./interface";
import { ObjectMap } from "@core/object";
import { Port } from "../command/nftables";
import { IpAddressWithPrefix } from "../command/ip";

const SCRIPT_PATH = path.join(path.dirname(fileURLToPath(import.meta.url)), "process.ts");
const TYPESCRIPT_EXECUTABLE = "tsx";

export class VRouterHandle {
    #controller: AbortController;
    #child: ChildProcess;

    constructor(args: { controller: AbortController; child: ChildProcess }) {
        this.#controller = args.controller;
        this.#child = args.child;
    }

    kill(): void {
        this.#controller.abort();
    }

    onExit(callback: () => void): void {
        this.#child.on("exit", callback);
    }
}

export class VRouterService {
    #network: NetNsManager;
    #interfaces = new VRouterInterfaceStore();
    #controllers = new ObjectMap<Port, AbortController, number>((port) => port.toNumber());

    private constructor(args: { network: NetNsManager }) {
        this.#network = args.network;
    }

    static async create(args: {
        vRouterLocalAddreessRange: IpAddressWithPrefix;
        rootBridgeLocalAddress: IpAddressWithPrefix;
    }) {
        return new VRouterService({ network: await NetNsManager.create(args) });
    }

    async spawn(): Promise<Port | undefined> {
        const interface_ = this.#interfaces.popNext();
        if (interface_ === undefined) {
            return;
        }

        const network = await this.#network.createVRouter(interface_);
        const controller = new AbortController();
        const child = spawn(TYPESCRIPT_EXECUTABLE, [SCRIPT_PATH], { signal: controller.signal });
        child.on("exit", () => {
            this.#controllers.delete(interface_.globalPort);
            this.#network.deleteVRouter(network);
            this.#interfaces.pushBack(interface_);
        });

        return interface_.globalPort;
    }

    kill(port: Port): boolean {
        const controller = this.#controllers.get(port);
        controller?.abort();
        return controller !== undefined;
    }

    getPorts(): Port[] {
        return Array.from(this.#controllers.keys());
    }
}
