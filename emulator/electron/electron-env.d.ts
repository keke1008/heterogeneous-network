/// <reference types="vite-plugin-electron/electron-env" />
import { Graph } from "./net";

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
            onGraphChanged: (callback: (graph: Graph) => void) => void;
            connect: (args: { address: string; port: string; cost: number }) => void;
            end: () => void;
        };
    }
}
