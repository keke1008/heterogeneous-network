import { Box, useTheme } from "@mui/material";
import { useEffect, useRef, useState } from "react";
import { CaptionRenderOptions } from "@core/apps/caption";
import { getCaptionRenderingPropeties, renderCaption } from "@emulator/apps/caption";

interface CaptionPreviewCanvasProps {
    options: CaptionRenderOptions;
    style: React.CSSProperties;
    onPointerDown?: (e: React.PointerEvent<HTMLCanvasElement>) => void;
    onPointerUp?: (e: React.PointerEvent<HTMLCanvasElement>) => void;
    onPointerMove?: (e: React.PointerEvent<HTMLCanvasElement>) => void;
    canvasRef: React.RefObject<HTMLCanvasElement>;
}

const CaptionPreviewCanvas: React.FC<CaptionPreviewCanvasProps> = ({
    options,
    style,
    onPointerDown,
    onPointerUp,
    onPointerMove,
    canvasRef,
}) => {
    useEffect(() => {
        if (canvasRef.current) {
            renderCaption(canvasRef.current, options);
        }

        const ctx = canvasRef.current?.getContext("2d");
        return () => {
            ctx?.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        };
    }, [canvasRef, options]);

    return (
        <canvas
            ref={canvasRef}
            style={style}
            onPointerDown={onPointerDown}
            onPointerUp={onPointerUp}
            onPointerMove={onPointerMove}
        />
    );
};

const useDrag = (element: HTMLElement | null, onDrag: (dx: number, dy: number) => void) => {
    type DragState = { state: "idle" } | { state: "dragging"; startX: number; startY: number };
    const [dragState, setDragState] = useState<DragState>({ state: "idle" });

    const handlePointerDown = (e: React.PointerEvent<HTMLCanvasElement>) => {
        setDragState({ state: "dragging", startX: e.clientX, startY: e.clientY });
        element?.setPointerCapture(e.pointerId);
    };

    const handlePointerUp = (e: React.PointerEvent<HTMLCanvasElement>) => {
        setDragState({ state: "idle" });
        element?.releasePointerCapture(e.pointerId);
    };

    const handlePointerMove = (e: React.PointerEvent<HTMLCanvasElement>) => {
        if (dragState.state !== "dragging") {
            return;
        }

        const dx = e.clientX - dragState.startX;
        const dy = e.clientY - dragState.startY;
        onDrag(dx, dy);
        setDragState({ state: "dragging", startX: e.clientX, startY: e.clientY });
    };

    return { handlePointerDown, handlePointerUp, handlePointerMove };
};

interface CaptionPreviewProps {
    displayWidth: number;
    displayHeight: number;
    options: CaptionRenderOptions;
    setOverflow?: (overflow: boolean) => void;
    onCaptionDrag?: (x: number, y: number) => void;
    onCaptionResize?: (fontSize: number) => void;
}

export const CaptionPreview: React.FC<CaptionPreviewProps> = ({
    displayWidth,
    displayHeight,
    options,
    setOverflow,
    onCaptionDrag,
    onCaptionResize,
}) => {
    const [captionWidth, setCaptionWidth] = useState(0);
    const [captionHeight, setCaptionHeight] = useState(0);

    useEffect(() => {
        const { width, height } = getCaptionRenderingPropeties(options);
        setCaptionWidth(width);
        setCaptionHeight(height);
    }, [options]);

    const captionXRatio = options.x / displayWidth;
    const captionYRatio = options.y / displayHeight;
    const captionWidthRatio = captionWidth / displayWidth;
    const captionHeightRatio = captionHeight / displayHeight;

    const widthOverflow = options.x < 0 || options.x + captionWidth > displayWidth;
    const heightOverflow = options.y < 0 || options.y + captionHeight > displayHeight;
    const overflow = widthOverflow || heightOverflow;
    useEffect(() => {
        setOverflow?.(overflow);
    }, [overflow, setOverflow]);

    const canvasRef = useRef<HTMLCanvasElement>(null);
    const { handlePointerDown, handlePointerUp, handlePointerMove } = useDrag(canvasRef.current, (dx, dy) => {
        const captionRect = canvasRef.current?.getBoundingClientRect();
        if (!captionRect) {
            return;
        }

        const captionXPixelsToReal = (displayWidth * captionWidthRatio) / captionRect.width;
        const captionYPixelsToReal = (displayHeight * captionHeightRatio) / captionRect.height;

        const newCaptionX = Math.floor(options.x + dx * captionXPixelsToReal);
        const newCaptionY = Math.floor(options.y + dy * captionYPixelsToReal);
        onCaptionDrag?.(newCaptionX, newCaptionY);
    });

    const displayRef = useRef<HTMLDivElement>(null);
    const handleWheel = (e: WheelEvent) => {
        if (!e.ctrlKey) {
            return;
        }

        e.preventDefault();

        const increment = 50;
        const delta = -Math.sign(e.deltaY) * increment;
        const newFontSize = Math.max(1, options.fontSize + Math.floor(delta));
        onCaptionResize?.(newFontSize);
    };

    useEffect(() => {
        const element = displayRef.current;
        element?.addEventListener("wheel", handleWheel, { passive: false });
        return () => {
            element?.removeEventListener("wheel", handleWheel);
        };
    });

    const theme = useTheme();

    return (
        <Box
            style={{
                aspectRatio: `${displayWidth} / ${displayHeight}`,
                borderStyle: "solid",
                borderWidth: 2,
                borderColor: overflow ? theme.palette.error.main : theme.palette.primary.main,
                overflow: "hidden",
            }}
            ref={displayRef}
        >
            <CaptionPreviewCanvas
                canvasRef={canvasRef}
                options={options}
                style={{
                    position: "relative",
                    left: `${captionXRatio * 100}%`,
                    top: `${captionYRatio * 100}%`,
                    width: `${captionWidthRatio * 100}%`,
                    height: `${captionHeightRatio * 100}%`,
                }}
                onPointerDown={handlePointerDown}
                onPointerUp={handlePointerUp}
                onPointerMove={handlePointerMove}
            />
        </Box>
    );
};
