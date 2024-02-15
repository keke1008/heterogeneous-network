import { ChatContextProvider } from "./Context";
import { Header } from "./Header";
import { History } from "./History";
import { Box, Stack } from "@mui/material";
import { SendMessage } from "./SendMessage";

export const ChatApp: React.FC = () => {
    return (
        <ChatContextProvider>
            <Stack height="100%">
                <Header />
                <Box sx={{ flexGrow: 1, marginX: 2, marginTop: 4, minHeight: 0 }}>
                    <Stack height="100%" spacing={2}>
                        <Box sx={{ flexGrow: 1, overflowY: "scroll" }}>
                            <History />
                        </Box>
                        <SendMessage />
                    </Stack>
                </Box>
            </Stack>
        </ChatContextProvider>
    );
};
