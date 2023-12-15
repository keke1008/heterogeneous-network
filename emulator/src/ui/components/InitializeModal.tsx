import { useContext, useState } from "react";
import { NetContext } from "../contexts/netContext";
import { Address, AddressType, Cost, SerialAddress } from "@core/net";
import { Button, Dialog, DialogActions, DialogContent, DialogTitle, Stack } from "@mui/material";
import { AddressInput, CostInput } from "./Input";

export const InitializeModal: React.FC = () => {
    const net = useContext(NetContext);
    const [serialAddress, setSerialAddress] = useState<Address | undefined>(undefined);
    const [cost, setCost] = useState<Cost | undefined>(new Cost(0));
    const [open, setOpen] = useState(() => net.localNode().id === undefined);

    const handleSubmit = () => {
        if (serialAddress === undefined || !(serialAddress.address instanceof SerialAddress)) {
            return;
        }
        net.registerFrameHandler({
            localSerialAddress: serialAddress.address,
            localCost: cost,
        });
        setOpen(false);
    };

    return (
        <Dialog open={open}>
            <DialogTitle>Initialize</DialogTitle>
            <DialogContent>
                <Stack spacing={2} marginY={2}>
                    <AddressInput
                        autoFocus
                        label="Local serial ID"
                        types={[AddressType.Serial]}
                        onChange={setSerialAddress}
                    />
                    <CostInput label="Local cost" onChange={setCost} />
                </Stack>
            </DialogContent>
            <DialogActions>
                <Button onClick={handleSubmit}>Submit</Button>
            </DialogActions>
        </Dialog>
    );
};
