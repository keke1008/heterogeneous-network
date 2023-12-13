import { exec } from "child_process";

export const executeCommand = async (command: string): Promise<string> => {
    const trimmed = command.replace(/^ +| +$/g, "");
    console.log(`[shell] "${trimmed}"`);
    return new Promise<string>((resolve, reject) => {
        exec(trimmed, (err, stdout, stderr) => (err ? reject(stderr) : resolve(stdout)));
    });
};
