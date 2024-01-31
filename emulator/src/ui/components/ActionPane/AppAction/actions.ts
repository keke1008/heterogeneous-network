import { Caption } from "./Caption";
import { Echo } from "./Echo";
import { FileAction } from "./File";

export const actions = [
    { name: "Echo", path: "echo", Component: Echo },
    { name: "Caption", path: "caption", Component: Caption },
    { name: "File", path: "file", Component: FileAction },
];
