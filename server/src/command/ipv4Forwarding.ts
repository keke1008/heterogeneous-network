import { executeCommand } from "./common";

export const setIpv4Forwarding = async (value: boolean) => {
    await executeCommand(`sysctl -w net.ipv4.ip_forward=${value ? 1 : 0}`);
};
