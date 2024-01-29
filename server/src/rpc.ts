import { RpcIgnoreRequest, RpcRequest, RpcRequestContext, RpcResponse, RpcServer, RpcStatus } from "@core/net";
import { Port, VRouterService } from "./vrouter";
import * as VRouter from "@core/net/rpc/procedures/vrouter/getVRouters";

export class GetVRoutersServer implements RpcServer {
    #vRouterService: VRouterService;

    constructor({ vRouterService }: { vRouterService: VRouterService }) {
        this.#vRouterService = vRouterService;
    }

    handleRequest(_: RpcRequest, ctx: RpcRequestContext): Promise<RpcResponse | RpcIgnoreRequest> {
        const ports = this.#vRouterService.getPorts();
        const result = ports.map((port) => ({ port: port.toNumber() }));
        return ctx.createResponse({
            status: RpcStatus.Success,
            serializer: VRouter.VRouters.serdeable.serializer(result),
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
                serializer: VRouter.VRouter.serdeable.serializer({ port: result.toNumber() }),
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
        const params = VRouter.VRouter.serdeable.deserializer().deserialize(request.bodyReader);
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
