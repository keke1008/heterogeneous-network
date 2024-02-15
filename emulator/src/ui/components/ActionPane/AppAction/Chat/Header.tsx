import { ChatRoom } from "@core/apps/chat";
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
    Select,
    MenuItem,
} from "@mui/material";
import { ChatAppContext, ChatConfigContext, ChatRoomContext } from "./context";
import { useContext, useState } from "react";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { ActionContext } from "@emulator/ui/contexts/actionContext";

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
                    value={config.aiImageServerAddress}
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

const Header: React.FC = () => {
    const app = useContext(ChatAppContext);
    const { target } = useContext(ActionContext);

    const handleConnect = async () => {
        const room = await app.connect(target);
        if (room.isOk()) {
            setRooms([...rooms, room.unwrap()]);
            setCurrentRoom(room.unwrap());
        }
    };

    const [openConfig, setOpenConfig] = useState(false);

    return (
        <>
            <ChatConfigDialog open={openConfig} onClose={() => setOpenConfig(false)} />
            <Grid container spacing={2}>
                <Grid item xs={8}>
                    <Select
                        value={currentRoom}
                        onChange={(e) => typeof e.target.value !== "string" && setCurrentRoom(e.target.value)}
                    >
                        {rooms.map((room) => (
                            <MenuItem key={room.remote.toString() + room.remotePortId.toString()} value={room}>
                                {room.remote.toString()}
                            </MenuItem>
                        ))}
                    </Select>
                </Grid>

                <Grid item xs={3}>
                    <Button fullWidth onClick={handleConnect}>
                        Connect
                    </Button>
                </Grid>

                <Grid item xs={1}>
                    <IconButton onClick={() => setOpenConfig(true)}>
                        <Settings />
                    </IconButton>
                </Grid>
            </Grid>
        </>
    );
};
