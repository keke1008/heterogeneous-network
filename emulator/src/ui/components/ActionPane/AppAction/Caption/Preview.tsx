import { Box, useTheme } from "@mui/material";
import { useEffect, useRef, useState } from "react";

interface CaptionPreviewCanvasProps {
    setWidth: (width: number) => void;
    setHeight: (height: number) => void;
    font: string;
    fontSize: number;
    color: string;
    text: string;
    style: React.CSSProperties;
    onPointerDown?: (e: React.PointerEvent<HTMLCanvasElement>) => void;
    onPointerUp?: (e: React.PointerEvent<HTMLCanvasElement>) => void;
    onPointerMove?: (e: React.PointerEvent<HTMLCanvasElement>) => void;
    canvasRef: React.RefObject<HTMLCanvasElement>;
}

const CaptionPreviewCanvas: React.FC<CaptionPreviewCanvasProps> = ({
    setWidth,
    setHeight,
    font,
    fontSize,
    color,
    text,
    style,
    onPointerDown,
    onPointerUp,
    onPointerMove,
    canvasRef,
}) => {
    useEffect(() => {
        const ctx = canvasRef.current?.getContext("2d");
        if (!ctx) {
            throw new Error("Failed to get canvas context");
        }

        ctx.font = `${fontSize}px ${font}`;
        const textMetrics = ctx.measureText(text);
        const width = textMetrics.width;
        const height = textMetrics.actualBoundingBoxAscent + textMetrics.actualBoundingBoxDescent;

        setWidth(width);
        setHeight(height);

        ctx.canvas.width = width;
        ctx.canvas.height = height;

        ctx.fillStyle = color;
        ctx.font = `${fontSize}px Arial`;
        ctx.fillText(text, 0, textMetrics.actualBoundingBoxAscent);

        return () => {
            ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        };
    }, [canvasRef, font, fontSize, color, text, setWidth, setHeight]);

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
    captionX: number;
    captionY: number;
    font: string;
    fontSize: number;
    color: string;
    text: string;
    setOverflow?: (overflow: boolean) => void;
    onCaptionDrag?: (x: number, y: number) => void;
    onCaptionResize?: (fontSize: number) => void;
}

export const CaptionPreview: React.FC<CaptionPreviewProps> = ({
    displayWidth,
    displayHeight,
    captionX,
    captionY,
    font,
    fontSize,
    color,
    text,
    setOverflow,
    onCaptionDrag,
    onCaptionResize,
}) => {
    const [captionWidth, setCaptionWidth] = useState(0);
    const [captionHeight, setCaptionHeight] = useState(0);

    const captionXRatio = captionX / displayWidth;
    const captionYRatio = captionY / displayHeight;
    const captionWidthRatio = captionWidth / displayWidth;
    const captionHeightRatio = captionHeight / displayHeight;

    const widthOverflow = captionX < 0 || captionX + captionWidth > displayWidth;
    const heightOverflow = captionY < 0 || captionY + captionHeight > displayHeight;
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

        const newCaptionX = Math.floor(captionX + dx * captionXPixelsToReal);
        const newCaptionY = Math.floor(captionY + dy * captionYPixelsToReal);
        onCaptionDrag?.(newCaptionX, newCaptionY);
    });

    const displayRef = useRef<HTMLDivElement>(null);
    const handleWheel = (e: WheelEvent) => {
        if (!e.ctrlKey) {
            return;
        }

        e.preventDefault();

        const newFontSize = Math.max(1, fontSize - e.deltaY);
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
            }}
            ref={displayRef}
        >
            <CaptionPreviewCanvas
                canvasRef={canvasRef}
                setWidth={setCaptionWidth}
                setHeight={setCaptionHeight}
                font={font}
                fontSize={fontSize}
                color={color}
                text={text}
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
