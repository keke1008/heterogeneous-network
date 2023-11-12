/// <reference types="vite-plugin-electron/electron-env" />

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
}
