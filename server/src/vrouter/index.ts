import * as path from "node:path";
import { spawn, ChildProcess } from "node:child_process";
import { Err, Ok, Result } from "oxide.ts";

const SCRIPT_PATH = path.join(__dirname, "launch.ts");
const TYPESCRIPT_EXECUTABLE = "tsx";

type Port = number;

export class VRouterHandle {
    #controller: AbortController;
    #child: ChildProcess;

    constructor(controller: AbortController, child: ChildProcess) {
        this.#controller = controller;
        this.#child = child;
    }

    kill(): void {
        this.#controller.abort();
    }

    onExit(callback: () => void): void {
        this.#child.on("exit", callback);
    }
}

export class VRouterSpawner {
    #handles = new Map<Port, VRouterHandle>();

    spawn(port: Port): Result<VRouterHandle, unknown> {
        if (typeof port !== "number") {
            return Err(new Error("port must be a number"));
        }

        if (this.#handles.has(port)) {
            return Err(new Error(`port ${port} is already in use`));
        }

        const args = [SCRIPT_PATH, port.toString()];
        const controller = new AbortController();
        const child = spawn(TYPESCRIPT_EXECUTABLE, args, { signal: controller.signal });
        const handle = new VRouterHandle(controller, child);
        this.#handles.set(port, handle);
        return Ok(handle);
    }

    kill(port: Port): void {
        const handle = this.#handles.get(port);
        handle?.kill();
        this.#handles.delete(port);
    }

    getPorts(): Port[] {
        return Array.from(this.#handles.keys());
    }
}
