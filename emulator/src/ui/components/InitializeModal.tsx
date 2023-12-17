import { useContext, useState } from "react";
import { NetContext } from "../contexts/netContext";
import { Address, AddressType, Cost, SerialAddress } from "@core/net";
import { Button, Dialog, DialogActions, DialogContent, DialogTitle, Stack } from "@mui/material";
import { AddressInput } from "./Input";
import { ZodSchemaInput } from "./Input/ZodSchemaInput";

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
                        textProps={{ autoFocus: true }}
                        label="Local serial ID"
                        types={[AddressType.Serial]}
                        onValue={setSerialAddress}
                        stringValue="1"
                    />
                    <ZodSchemaInput
                        schema={Cost.schema}
                        onValue={setCost}
                        stringValue="0"
                        label="Local cost"
                        type="number"
                    />
                </Stack>
            </DialogContent>
            <DialogActions>
                <Button onClick={handleSubmit}>Submit</Button>
            </DialogActions>
        </Dialog>
    );
};
