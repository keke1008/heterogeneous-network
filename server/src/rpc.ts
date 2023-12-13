import { RpcIgnoreRequest, RpcRequest, RpcRequestContext, RpcResponse, RpcServer, RpcStatus } from "@core/net";
import { VRouterService } from "./vrouter";
import { Port } from "./command/nftables";
import * as VRouter from "@core/net/rpc/procedures/vrouter/getVRouters";

export class GetVRoutersServer implements RpcServer {
    #vRouterService: VRouterService;

    constructor({ vRouterService }: { vRouterService: VRouterService }) {
        this.#vRouterService = vRouterService;
    }

    handleRequest(_: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const ports = this.#vRouterService.getPorts();
        const result = ports.map((port) => new VRouter.VRouter({ port: port.toNumber() }));
        return ctx.createResponse({
            status: RpcStatus.Success,
            serializable: VRouter.VRouters.serializable(result),
        });
    }
}

export class CreateVRouterServer implements RpcServer {
    #vRouterService: VRouterService;

    constructor({ vRouterService }: { vRouterService: VRouterService }) {
        this.#vRouterService = vRouterService;
    }

    async handleRequest(_: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const result = await this.#vRouterService.spawn();
        if (result === undefined) {
            return ctx.createResponse({ status: RpcStatus.Failed });
        } else {
            return ctx.createResponse({
                status: RpcStatus.Success,
                serializable: new VRouter.VRouter({ port: result.toNumber() }),
            });
        }
    }
}

export class DeleteVRouterServer implements RpcServer {
    #vRouterService: VRouterService;

    constructor({ vRouterService }: { vRouterService: VRouterService }) {
        this.#vRouterService = vRouterService;
    }

    handleRequest(request: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const params = VRouter.VRouter.deserialize(request.bodyReader);
        if (params.isErr()) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }

        const port = Port.schema.safeParse(params.unwrap().port);
        if (!port.success) {
            return ctx.createResponse({ status: RpcStatus.BadArgument });
        }

        const result = this.#vRouterService.kill(port.data);
        if (!result) {
            return ctx.createResponse({ status: RpcStatus.Failed });
        }

        return ctx.createResponse({ status: RpcStatus.Success });
    }
}
