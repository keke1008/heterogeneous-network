export type Progress =
    | { type: "progress"; doneByteLength: number; totalByteLength: number }
    | { type: "complete" }
    | { type: "error"; error: string };
