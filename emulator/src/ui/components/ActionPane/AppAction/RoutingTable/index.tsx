import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { RoutingTableClient } from "@vrouter/apps/routing-table";
import { useContext, useState } from "react";
import { RoutingTable } from "./Table";
import { ConnectionButton } from "../ConnectionButton";
import { RoutingEntry } from "@vrouter/routing";

interface InnerAppProps {
    client: RoutingTableClient;
}
const InnerApp: React.FC<InnerAppProps> = ({ client }) => {
    const [entries, setEntries] = useState<RoutingEntry[]>([]);
    client.onUpdateEntries(setEntries);

    const handleUpdate = (entry: RoutingEntry) => {
        client.updateEntries([entry]);
    };
    const handleDelete = (entry: RoutingEntry) => {
        client.deleteEntries([entry.matcher]);
    };

    return <RoutingTable entries={entries} onUpdate={handleUpdate} onDelete={handleDelete} />;
};

export const RoutingTableApp: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const connect = async () => {
        const client = await RoutingTableClient.connect({ streamService: net.stream(), destination: target });
        return client.isOk()
            ? ({ type: "success", client: client.unwrap() } as const)
            : ({ type: "failure", reason: `${client.unwrapErr()}` } as const);
    };

    return <ConnectionButton connect={connect}>{(client) => <InnerApp client={client} />}</ConnectionButton>;
};
