import { Connection } from "./Connection";
import { Blink } from "./Debug";
import { GetMediaList } from "./Media";
import { SendHello } from "./Neighbor/SendHello";
import { CreateVRouter, DeleteVRouter, GetVRouters } from "./VRouter";
import { SetSerialAddress } from "./Serial";
import { CloseServer, ConnectToAccessPoint, StartServer } from "./Wifi";
import { SetClusterId, SetCost } from "./Local";
import { SetEthernetIpAddress, SetEthernetSubnetMask } from "./Ethernet";

export const actionGroups = [
    {
        groupName: "Connection",
        actions: [{ name: "Connect", path: "connect", Component: Connection }],
    },
    {
        groupName: "Debug",
        actions: [{ name: "Blink", path: "blink", Component: Blink }],
    },
    {
        groupName: "Media",
        actions: [{ name: "Get media list", path: "get-media-list", Component: GetMediaList }],
    },
    {
        groupName: "Serial",
        actions: [{ name: "Set serial address", path: "set-serial-address", Component: SetSerialAddress }],
    },
    {
        groupName: "Wifi",
        actions: [
            { name: "Connect to access point", path: "connect-to-access-point", Component: ConnectToAccessPoint },
            { name: "Start server", path: "start-server", Component: StartServer },
            { name: "Close server", path: "close-server", Component: CloseServer },
        ],
    },
    {
        groupName: "Ethernet",
        actions: [
            { name: "Set ethernet ip address", path: "set-ethernet-ip-address", Component: SetEthernetIpAddress },
            { name: "Set ethernet subnet mask", path: "set-ethernet-subnet-mask", Component: SetEthernetSubnetMask },
        ],
    },
    {
        groupName: "Local",
        actions: [
            { name: "Set cluster id", path: "set-cluster-id", Component: SetClusterId },
            { name: "Set cost", path: "set-cost", Component: SetCost },
        ],
    },
    {
        groupName: "Neighbor",
        actions: [{ name: "Send hello", path: "send-hello", Component: SendHello }],
    },
    {
        groupName: "VRouter",
        actions: [
            { name: "Create vrouter", path: "create-vrouter", Component: CreateVRouter },
            { name: "Delete vrouter", path: "delete-vrouter", Component: DeleteVRouter },
            { name: "Get vrouters", path: "get-vrouters", Component: GetVRouters },
        ],
    },
];

export const routes = actionGroups.flatMap((group) => {
    return group.actions.map(({ path, Component }) => ({ path, Component }));
});
