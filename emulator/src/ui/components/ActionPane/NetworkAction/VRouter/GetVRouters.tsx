import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { Stack, Table, TableBody, TableCell, TableContainer, TableHead, TableRow } from "@mui/material";
import { ActionRpcButton } from "../../ActionTemplates";
import { VRouters } from "@core/net/rpc/procedures/vrouter/getVRouters";
import { RpcStatus } from "@core/net";

export const GetVRouters: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [vRouters, setVRouters] = useState<VRouters>();

    const getVRouters = async () => {
        const result = await net.rpc().requestGetVRouters(target);
        if (result.status === RpcStatus.Success) {
            setVRouters(result.value);
        } else {
            setVRouters([]);
        }
        return result;
    };

    return (
        <Stack spacing={2}>
            <ActionRpcButton onClick={getVRouters}>Get vrouters</ActionRpcButton>

            {vRouters && (
                <TableContainer>
                    <Table>
                        <TableHead>
                            <TableRow>
                                <TableCell>#</TableCell>
                                <TableCell align="center">Port</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {vRouters.map((vRouter, index) => (
                                <TableRow key={index}>
                                    <TableCell>{index}</TableCell>
                                    <TableCell align="center">{vRouter.port}</TableCell>
                                </TableRow>
                            ))}
                        </TableBody>
                    </Table>
                </TableContainer>
            )}
        </Stack>
    );
};
