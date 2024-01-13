import { CaptionClient, ServerInfo, ClearCaption, CaptionStatus } from "@core/apps/caption";
import { ActionResult, ActionButton } from "../../ActionTemplates";

interface ClearCaptionInputProps {
    client: CaptionClient;
    server: ServerInfo;
}

export const ClearCaptionInput: React.FC<ClearCaptionInputProps> = ({ client, server }) => {
    const handleClick = async (): Promise<ActionResult> => {
        const result = await client.send(new ClearCaption({ server }));
        return result.status === CaptionStatus.Success ? { type: "success" } : { type: "failure", reason: "Failure" };
    };

    return <ActionButton onClick={handleClick}>Clear caption</ActionButton>;
};
