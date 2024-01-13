import { CaptionClient, ServerInfo, ShowCaption, CaptionStatus } from "@core/apps/caption";
import { Autocomplete, Grid, TextField } from "@mui/material";
import { useCallback, useMemo, useReducer, useState } from "react";
import { ActionResult, ActionButton } from "../../ActionTemplates";
import { CaptionPreview } from "./Preview";
import { z } from "zod";

interface ShowCaptionInputProps {
    client: CaptionClient;
    server: ServerInfo;
}

const captionInputSchema = z.object({
    displayWidth: z.coerce.number(),
    displayHeight: z.coerce.number(),
    x: z.coerce.number(),
    y: z.coerce.number(),
    font: z.string(),
    fontSize: z.coerce.number(),
    color: z.string(),
    text: z.string(),
});

type Resolution = { label: string; width: number; height: number };
const resolutions: Resolution[] = [
    { label: "Full HD", width: 1920, height: 1080 },
    { label: "4K", width: 3840, height: 2160 },
    { label: "8K", width: 7680, height: 4320 },
];
const initialResolution = resolutions[0];

export const ShowCaptionInput: React.FC<ShowCaptionInputProps> = ({ client, server }) => {
    const [input, setInput] = useState({
        displayWidth: `${initialResolution.width}`,
        displayHeight: `${initialResolution.height}`,
        x: "0",
        y: "0",
        font: "Arial",
        fontSize: "256",
        color: "#00ff80",
        text: "Hello, world!",
        overflow: false,
    });

    const parsed = useMemo(() => captionInputSchema.safeParse(input), [input]);

    const setOverflow = useCallback((overflow: boolean) => setInput((input) => ({ ...input, overflow })), []);

    const [resolution, setResolution] = useReducer((_: Resolution, value: Resolution | null) => {
        const next = value ?? initialResolution;
        setInput((input) => ({ ...input, displayWidth: `${next.width}`, displayHeight: `${next.height}` }));
        return next;
    }, initialResolution);

    const handleClick = async (): Promise<ActionResult> => {
        if (!parsed.success) {
            return { type: "failure", reason: parsed.error.message };
        }
        const caption = new ShowCaption({ server, ...parsed.data });
        const result = await client.send(caption);
        return result.status === CaptionStatus.Success ? { type: "success" } : { type: "failure", reason: "Failure" };
    };

    return (
        <Grid container spacing={2}>
            <Grid item xs={12}>
                <CaptionPreview
                    displayWidth={parsed.success ? parsed.data.displayWidth : initialResolution.width}
                    displayHeight={parsed.success ? parsed.data.displayHeight : initialResolution.height}
                    captionX={parsed.success ? parsed.data.x : 0}
                    captionY={parsed.success ? parsed.data.y : 0}
                    font={parsed.success ? parsed.data.font : ""}
                    fontSize={parsed.success ? parsed.data.fontSize : 0}
                    color={parsed.success ? parsed.data.color : ""}
                    text={parsed.success ? parsed.data.text : ""}
                    onCaptionDrag={(x, y) => setInput((input) => ({ ...input, x: `${x}`, y: `${y}` }))}
                    onCaptionResize={(fontSize) => setInput((input) => ({ ...input, fontSize: `${fontSize}` }))}
                    setOverflow={setOverflow}
                />
            </Grid>
            <Grid item xs={4}>
                <TextField
                    required
                    fullWidth
                    type="number"
                    label="x"
                    value={input.x}
                    onChange={(e) => setInput({ ...input, x: e.target.value })}
                />
            </Grid>
            <Grid item xs={4}>
                <TextField
                    required
                    fullWidth
                    type="number"
                    label="y"
                    value={input.y}
                    onChange={(e) => setInput({ ...input, y: e.target.value })}
                />
            </Grid>
            <Grid item xs={4}>
                <TextField
                    required
                    fullWidth
                    type="number"
                    id="fontSize"
                    label="font size"
                    value={input.fontSize}
                    onChange={(e) => setInput({ ...input, fontSize: e.target.value })}
                />
            </Grid>
            <Grid item xs={4}>
                <TextField
                    required
                    fullWidth
                    type="color"
                    id="color"
                    label="color"
                    value={input.color}
                    onChange={(e) => setInput({ ...input, color: e.target.value })}
                />
            </Grid>
            <Grid item xs={4}>
                <TextField
                    required
                    fullWidth
                    type="text"
                    id="font"
                    label="font"
                    value={input.font}
                    onChange={(e) => setInput({ ...input, font: e.target.value })}
                />
            </Grid>
            <Grid item xs={4}>
                <Autocomplete
                    fullWidth
                    id="resolution"
                    value={resolution}
                    options={resolutions}
                    onChange={(_, value) => setResolution(value)}
                    renderInput={(params) => <TextField required label="resolution" {...params} />}
                    isOptionEqualToValue={(option, value) => option.width === value.width}
                />
            </Grid>

            <Grid item xs={12}>
                <TextField
                    required
                    fullWidth
                    id="text"
                    label="text"
                    value={input.text}
                    onChange={(e) => setInput({ ...input, text: e.target.value })}
                />
            </Grid>
            <Grid item xs={12}>
                <ActionButton onClick={handleClick}>Show caption</ActionButton>
            </Grid>
        </Grid>
    );
};
