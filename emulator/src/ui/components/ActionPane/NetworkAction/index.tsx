import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { Typography, Stack, Divider } from "@mui/material";
import { useContext } from "react";
import { Debug } from "./Debug";
import { Connection } from "./Connection";
import { Media } from "./Media";
import { Neighbor } from "./Neighbor";
import { Serial } from "./Serial";
import { VRouter } from "./VRouter";
import { Wifi } from "./Wifi";
import { Local } from "./Local";

export const NetworkAction: React.FC = () => {
    const { target } = useContext(ActionContext);

    return (
        target && (
            <>
                <Typography variant="h4" sx={{ textAlign: "center" }}>
                    {target?.nodeId.toString()}, {target?.clusterId.toString()}
                </Typography>

                <Stack spacing={1} paddingY={1} divider={<Divider />}>
                    <Connection />
                    <Debug />
                    <Media />
                    <Wifi />
                    <Serial />
                    <Local />
                    <Neighbor />
                    <VRouter />
                </Stack>
            </>
        )
    );
};
