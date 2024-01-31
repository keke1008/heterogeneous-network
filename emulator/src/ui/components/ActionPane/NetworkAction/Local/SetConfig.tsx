import { Config, RpcResult } from "@core/net";
import { ActionContext } from "@emulator/ui/contexts/actionContext";
import { NetContext } from "@emulator/ui/contexts/netContext";
import { useContext, useState } from "react";
import { ActionRpcForm } from "../../ActionTemplates";
import { Checkbox, FormControlLabel } from "@mui/material";

const configs: Config[] = Object.values(Config).filter((v): v is number => typeof v === "number");

export const SetConfig: React.FC = () => {
    const net = useContext(NetContext);
    const { target } = useContext(ActionContext);

    const [config, setConfig] = useState<Config | 0>(0);
    const setConfigAction = async (): Promise<RpcResult<unknown>> => {
        return net.rpc().requestSetConfig(target, config);
    };

    const onChange = (c: Config) => (e: React.ChangeEvent<HTMLInputElement>) => {
        e.target.checked ? setConfig(config | c) : setConfig(config & ~c);
    };

    const selection = configs.map((c) => ({
        name: Config[c],
        value: c,
        checked: (config & c) !== 0,
    }));

    return (
        <ActionRpcForm onSubmit={setConfigAction} submitButtonText="Set config">
            {selection.map((c) => (
                <FormControlLabel
                    key={c.name}
                    control={<Checkbox checked={c.checked} onChange={onChange(c.value)}></Checkbox>}
                    label={c.name}
                />
            ))}
        </ActionRpcForm>
    );
};
