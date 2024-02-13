import * as https from "node:https";
import * as http from "node:http";
import * as fs from "node:fs";
import * as path from "node:path";
import { fileURLToPath } from "node:url";
import * as dotenv from "dotenv";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, "..", "..");
dotenv.config({ path: path.join(repoRoot, ".env") });

interface SslOptions {
    cert?: string;
    key?: string;
    ca?: string;
}

const sslOptions: SslOptions = {
    cert: process.env.SSL_CERT_PATH,
    key: process.env.SSL_KEY_PATH,
    ca: process.env.SSL_CA_PATH,
};

const useSsl = (sslOptions: SslOptions): sslOptions is Required<SslOptions> => {
    return sslOptions.cert !== undefined && sslOptions.key !== undefined;
};

if (useSsl(sslOptions)) {
    console.info("using HTTPS");
    if (!fs.existsSync(sslOptions.cert)) {
        throw new Error(`SSL cert not found: ${sslOptions.cert}`);
    }
    if (!fs.existsSync(sslOptions.key)) {
        throw new Error(`SSL key not found: ${sslOptions.key}`);
    }
    if (sslOptions.ca !== undefined && !fs.existsSync(sslOptions.ca)) {
        throw new Error(`SSL CA not found: ${sslOptions.ca}`);
    }
} else {
    console.info("using HTTP");
}

export const httpsServerOptions: https.ServerOptions | undefined = useSsl(sslOptions)
    ? {
          cert: fs.readFileSync(sslOptions.cert),
          key: fs.readFileSync(sslOptions.key),
          ca: sslOptions.ca !== undefined ? fs.readFileSync(sslOptions.ca) : undefined,
      }
    : undefined;

export type Server = http.Server | https.Server;

export const createServer = (requestListener?: http.RequestListener): Server => {
    return httpsServerOptions !== undefined
        ? https.createServer(httpsServerOptions, requestListener)
        : http.createServer(requestListener);
};
