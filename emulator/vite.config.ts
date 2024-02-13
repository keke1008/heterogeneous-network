import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tsconfigPaths from "vite-tsconfig-paths";
import { httpsServerOptions } from "../ts-core/src/httpServer";

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [tsconfigPaths(), react()],
    build: {
        sourcemap: true,
    },
    server: {
        https: httpsServerOptions,
    },
});
