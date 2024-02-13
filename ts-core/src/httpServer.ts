import * as https from "node:https";
import * as http from "node:http";
import * as fs from "node:fs";
import * as dotenv from "dotenv";

dotenv.config();

interface SslOptions {
    cert?: string;
    key?: string;
}

const sslOptions: SslOptions = {
    cert: process.env.SSL_CERT_PATH,
    key: process.env.SSL_KEY_PATH,
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
} else {
    console.info("using HTTP");
}

export const httpsServerOptions: https.ServerOptions | undefined = useSsl(sslOptions)
    ? { cert: fs.readFileSync(sslOptions.cert), key: fs.readFileSync(sslOptions.key) }
    : undefined;

export type Server = http.Server | https.Server;

export const createServer = (requestListener?: http.RequestListener): Server => {
    return httpsServerOptions !== undefined
        ? https.createServer(httpsServerOptions, requestListener)
        : http.createServer(requestListener);
};
