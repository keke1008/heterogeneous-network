import { exec } from "child_process";

export const executeCommand = async (command: string): Promise<string> => {
    console.log(`[shell] "${command}"`);
    return new Promise<string>((resolve, reject) => {
        exec(command, (err, stdout, stderr) => (err ? reject(stderr) : resolve(stdout)));
    });
};
