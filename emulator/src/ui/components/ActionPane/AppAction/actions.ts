import { AiImage } from "./AiImage";
import { Caption } from "./Caption";
import { ChatApp } from "./Chat";
import { Echo } from "./Echo";
import { FileAction } from "./File";
import { PostingApp } from "./Posting";
import { RoutingTableApp } from "./RoutingTable";

export const actions = [
    { name: "Echo", path: "echo", Component: Echo },
    { name: "Caption", path: "caption", Component: Caption },
    { name: "File", path: "file", Component: FileAction },
    { name: "RoutingTable", path: "routing-table", Component: RoutingTableApp },
    { name: "AiImage", path: "ai-image", Component: AiImage },
    { name: "Chat", path: "chat", Component: ChatApp },
    { name: "Posting", path: "posting", Component: PostingApp },
];
