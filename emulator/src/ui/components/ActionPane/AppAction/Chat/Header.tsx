import { Settings } from "@mui/icons-material";
import {
    Grid,
    Button,
    IconButton,
    Dialog,
    DialogActions,
    DialogContent,
    DialogTitle,
    TextField,
    Autocomplete,
} from "@mui/material";
import { ChatAppContext, ChatConfigContext, ChatRoomContext } from "./Context";
import { useContext, useState } from "react";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { LoadingButton } from "@mui/lab";

interface ChatConfigDialogProps {
    open: boolean;
    onClose(): void;
}

export const ChatConfigDialog: React.FC<ChatConfigDialogProps> = ({ open, onClose }) => {
    const { config, setConfig } = useContext(ChatConfigContext);

    return (
        <Dialog open={open} onClose={onClose}>
            <DialogTitle>Chat Config</DialogTitle>
            <DialogContent>
                <TextField
                    fullWidth
                    label="AI Image Server Address"
                    value={config.aiImageServerAddress ?? ""}
                    onChange={(e) => {
                        setConfig({ ...config, aiImageServerAddress: e.target.value });
                    }}
                />
            </DialogContent>
            <DialogActions>
                <Button onClick={onClose}>Cancel</Button>
                <Button onClick={onClose}>Save</Button>
            </DialogActions>
        </Dialog>
    );
};

export const Header: React.FC = () => {
    const app = useContext(ChatAppContext);
    const { target } = useContext(ActionContext);
    const { rooms, currentRoom, setCurrentRoom } = useContext(ChatRoomContext);

    const [disconnecting, setDisconnecting] = useState(false);
    const handleDisconnect = async () => {
        if (currentRoom === undefined) {
            return;
        }

        setDisconnecting(true);
        await currentRoom?.close();
        setDisconnecting(false);
    };

    const [connecting, setConnecting] = useState(false);
    const handleConnect = async () => {
        setConnecting(true);
        const room = await app.connect(target);
        room.isOk() && setCurrentRoom(room.unwrap());
        setConnecting(false);
    };

    const [openConfig, setOpenConfig] = useState(false);

    return (
        <>
            <ChatConfigDialog open={openConfig} onClose={() => setOpenConfig(false)} />
            <Grid container spacing={2} alignItems="center">
                <Grid item xs>
                    <Autocomplete
                        fullWidth
                        disabled={connecting || disconnecting}
                        value={currentRoom ?? null}
                        options={rooms}
                        getOptionLabel={(option) => `${option.remote.display()}, port:${option.remotePortId.display()}`}
                        onChange={(_, value) => setCurrentRoom(value ?? undefined)}
                        renderInput={(params) => <TextField {...params} label="Room" />}
                        isOptionEqualToValue={(option, value) => option.uuid === value.uuid}
                        getOptionKey={(option) => option.uuid}
                    />
                </Grid>

                <Grid item>
                    <LoadingButton
                        loading={disconnecting}
                        fullWidth
                        onClick={handleDisconnect}
                        color="error"
                        disabled={currentRoom === undefined}
                    >
                        Disconnect
                    </LoadingButton>
                </Grid>

                <Grid item>
                    <LoadingButton loading={connecting} fullWidth onClick={handleConnect}>
                        Connect
                    </LoadingButton>
                </Grid>

                <Grid item>
                    <IconButton onClick={() => setOpenConfig(true)}>
                        <Settings />
                    </IconButton>
                </Grid>
            </Grid>
        </>
    );
};
