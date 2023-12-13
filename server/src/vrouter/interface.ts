import { IpAddressWithPrefix } from "../command/ip";
import { Port } from "../command/nftables";

export interface VRouterInterface {
    readonly globalPort: Port;
    readonly localAddress: IpAddressWithPrefix;
}

export const createDefaultVRouterInterfaces = (): VRouterInterface[] => {
    const interfaces: VRouterInterface[] = [];

    const MAX_INTERFACES = 5;
    for (let i = 0; i < MAX_INTERFACES; ++i) {
        const globalPort = 10000 + i;
        const localAddress = `10.0.0.${i}/24`;
        interfaces.push({
            globalPort: Port.schema.parse(globalPort),
            localAddress: IpAddressWithPrefix.schema.parse(localAddress),
        });
    }

    return interfaces;
};

export class VRouterInterfaceStore {
    #interfaces = createDefaultVRouterInterfaces();

    popNext(): VRouterInterface | undefined {
        return this.#interfaces.shift();
    }

    pushBack(interface_: VRouterInterface): void {
        this.#interfaces.push(interface_);
    }
}
