import { AppContext } from "@emulator/ui/contexts/appContext";
import { enqueueSnackbar } from "notistack";
import { useContext, useEffect } from "react";

export const usePosting = () => {
    const net = useContext(AppContext);
    const server = net.postingServer();
    useEffect(() => {
        return server.onReceive((data) => {
            enqueueSnackbar(`Posting: ${data}`);
        });
    }, [server]);
};
