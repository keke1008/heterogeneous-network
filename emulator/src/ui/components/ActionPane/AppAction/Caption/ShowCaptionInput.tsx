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
    overflow: z.boolean(),
});

type Font = { label: string; generic: string };
const fontOptions: Font[] = [
    { label: "Arial", generic: "sans-serif" },
    { label: "Verdana", generic: "sans-serif" },
    { label: "Tahoma", generic: "sans-serif" },
    { label: "Trebuchet MS", generic: "sans-serif" },
    { label: "Times New Roman", generic: "serif" },
    { label: "Georgia", generic: "serif" },
    { label: "Garmond", generic: "serif" },
    { label: "Courier New", generic: "monospace" },
    { label: "Brush Script MT", generic: "cursive" },
];
const initialFont = fontOptions[0];

type Resolution = { label: string; width: number; height: number };
const resolutions: Resolution[] = [
    { label: "Full HD", width: 1920, height: 1080 },
    { label: "4K", width: 3840, height: 2160 },
    { label: "8K", width: 7680, height: 4320 },
];
const initialResolution = resolutions[0];

const initialInputState = {
    displayWidth: `${initialResolution.width}`,
    displayHeight: `${initialResolution.height}`,
    x: "0",
    y: "0",
    font: "Arial",
    fontSize: "256",
    color: "#00ff80",
    text: "Hello, world!",
    overflow: false,
};
const initialParsedInputState = captionInputSchema.parse(initialInputState);

export const ShowCaptionInput: React.FC<ShowCaptionInputProps> = ({ client, server }) => {
    const [input, setInput] = useState(initialInputState);

    const parsed = useMemo(() => captionInputSchema.safeParse(input), [input]);

    const setOverflow = useCallback((overflow: boolean) => setInput((input) => ({ ...input, overflow })), []);

    const [font, setFont] = useReducer((_: string | Font, value: string | Font | null) => {
        const font = typeof value === "string" ? value : value?.label ?? "";
        setInput((input) => ({ ...input, font }));
        return value ?? "";
    }, initialFont);

    const [resolution, setResolution] = useReducer((_: Resolution, value: Resolution | null) => {
        const next = value ?? initialResolution;
        setInput((input) => ({ ...input, displayWidth: `${next.width}`, displayHeight: `${next.height}` }));
        return next;
    }, initialResolution);

    const handleClick = async (): Promise<ActionResult> => {
        if (!parsed.success) {
            return { type: "failure", reason: parsed.error.message };
        }
        if (parsed.data.overflow) {
            return { type: "failure", reason: "Overflow" };
        }

        const caption = new ShowCaption({ server, options: parsed.data });
        const result = await client.send(caption);
        return result.status === CaptionStatus.Success ? { type: "success" } : { type: "failure", reason: "Failure" };
    };

    return (
        <Grid container spacing={2}>
            <Grid item xs={12}>
                <CaptionPreview
                    displayWidth={parsed.success ? parsed.data.displayWidth : initialResolution.width}
                    displayHeight={parsed.success ? parsed.data.displayHeight : initialResolution.height}
                    options={parsed.success ? parsed.data : initialParsedInputState}
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
                <Autocomplete
                    fullWidth
                    freeSolo
                    id="font"
                    value={font}
                    options={fontOptions}
                    onInputChange={(_, value) => setFont(value)}
                    renderInput={(params) => <TextField required label="font" {...params} />}
                    groupBy={(option) => option.generic}
                    isOptionEqualToValue={(option, value) => option.label === value.label}
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
