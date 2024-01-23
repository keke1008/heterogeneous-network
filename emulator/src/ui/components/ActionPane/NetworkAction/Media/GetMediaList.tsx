import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { ActionRpcButton } from "../../ActionTemplates/ActionButton";
import { Stack, Table, TableBody, TableCell, TableContainer, TableHead, TableRow } from "@mui/material";
import { AddressType, MediaInfo, RpcStatus } from "@core/net";

export const GetMediaList: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [mediaList, setMediaList] = useState<MediaInfo[]>();
    const rows = mediaList?.map((info, index) => ({
        index,
        addressType: (info.addressType && AddressType[info.addressType]) ?? "-",
        address: (info.address && info.address.address.toString()) ?? "-",
    }));

    const getMediaList = async () => {
        const result = await net.rpc().requestGetMediaList(target);
        if (result.status === RpcStatus.Success) {
            setMediaList(result.value);
        } else {
            setMediaList(undefined);
        }
        return result;
    };

    return (
        <Stack spacing={4}>
            <ActionRpcButton onClick={getMediaList}>Get media list</ActionRpcButton>

            {rows && (
                <TableContainer>
                    <Table>
                        <TableHead>
                            <TableRow>
                                <TableCell>Index</TableCell>
                                <TableCell align="center">Address Type</TableCell>
                                <TableCell align="center">Address</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {rows.map((row) => (
                                <TableRow key={row.index}>
                                    <TableCell>{row.index}</TableCell>
                                    <TableCell align="center">{row.addressType}</TableCell>
                                    <TableCell align="center">{row.address}</TableCell>
                                </TableRow>
                            ))}
                        </TableBody>
                    </Table>
                </TableContainer>
            )}
        </Stack>
    );
};
