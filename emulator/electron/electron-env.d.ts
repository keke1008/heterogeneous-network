/// <reference types="vite-plugin-electron/electron-env" />
import type { ModifyResult } from "./net";
import type { IpcRendererEvent } from "electron";

declare global {
    declare namespace NodeJS {
        interface ProcessEnv {
            /**
             * The built directory structure
             *
             * ```tree
             * ├─┬─┬ dist
             * │ │ └── index.html
             * │ │
             * │ ├─┬ dist-electron
             * │ │ ├── main.js
             * │ │ └── preload.js
             * │
             * ```
             */
            DIST: string;
            /** /dist/ or /public/ */
            VITE_PUBLIC: string;
        }
    }

    // Used in Renderer process, expose in `preload.ts`
    interface Window {
        net: {
            begin: (args: { selfAddress: string; selfPort: string }) => void;
            onGraphModified: (callback: (event: IpcRendererEvent, result: ModifyResult) => void) => void;
            connect: (args: { address: string; port: string; cost: number }) => void;
            end: () => void;
        };
    }
}
