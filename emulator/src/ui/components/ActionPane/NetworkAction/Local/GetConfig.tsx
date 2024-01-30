import { Config, RpcStatus } from "@core/net";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { Stack, TableContainer, Table, TableHead, TableRow, TableCell, TableBody } from "@mui/material";
import { useContext, useState } from "react";
import { ActionRpcButton } from "../../ActionTemplates";

export const GetConfig: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [config, setConfig] = useState<Config>();
    const rows =
        config === undefined
            ? undefined
            : Object.values(Config)
                  .filter((key): key is number => typeof key === "number")
                  .map((key) => ({
                      name: Config[key],
                      value: (config & key) !== 0 ? "true" : "false",
                  }));

    const getConfig = async () => {
        const result = await net.rpc().requestGetConfig(target);
        if (result.status === RpcStatus.Success) {
            setConfig(result.value);
        } else {
            setConfig(undefined);
        }
        return result;
    };

    return (
        <Stack spacing={4}>
            <ActionRpcButton onClick={getConfig}>Get config</ActionRpcButton>

            {rows !== undefined && (
                <TableContainer>
                    <Table>
                        <TableHead>
                            <TableRow>
                                <TableCell>Name</TableCell>
                                <TableCell align="center">Value</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {rows.map((row) => (
                                <TableRow key={row.name}>
                                    <TableCell>{row.name}</TableCell>
                                    <TableCell align="center">{row.value}</TableCell>
                                </TableRow>
                            ))}
                        </TableBody>
                    </Table>
                </TableContainer>
            )}
        </Stack>
    );
};
