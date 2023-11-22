import { Address, AddressError, AddressType } from "@core/net/link";
import { RpcClient } from "../handler";
import { NodeId, ReactiveService } from "@core/net/routing";
import { Procedure, RpcRequest, RpcResponse, RpcStatus } from "../../frame";
import { RequestManager, RpcResult } from "../../request";
import { BufferReader } from "@core/net/buffer";
import { Result } from "oxide.ts";
import { DeserializeOptional, DeserializeU8, DeserializeVector } from "@core/serde";

export class MediaInfo {
    addressType: AddressType | undefined;
    address: Address | undefined;

    constructor(args: { addressType: AddressType | undefined; address: Address | undefined }) {
        this.addressType = args.addressType;
        this.address = args.address;
    }

    static deserialize(reader: BufferReader): Result<MediaInfo, AddressError> {
        const addressType = new DeserializeOptional(DeserializeU8).deserialize(reader).unwrap();
        return new DeserializeOptional(Address)
            .deserialize(reader)
            .map((address) => new MediaInfo({ addressType, address }));
    }
}

class MediaList {
    list: MediaInfo[];

    constructor(args: { list: MediaInfo[] }) {
        this.list = args.list;
    }

    static deserialize(reader: BufferReader): Result<MediaList, AddressError> {
        return new DeserializeVector(MediaInfo).deserialize(reader).map((list) => new MediaList({ list }));
    }
}

export class Client implements RpcClient<MediaInfo[]> {
    #requestManager: RequestManager<MediaInfo[]>;

    constructor({ reactiveService }: { reactiveService: ReactiveService }) {
        this.#requestManager = new RequestManager({ procedure: Procedure.GetMediaList, reactiveService });
    }

    createRequest(destinationId: NodeId): [RpcRequest, Promise<RpcResult<MediaInfo[]>>] {
        return this.#requestManager.createRequest(destinationId);
    }

    handleResponse(response: RpcResponse): void {
        const list = MediaList.deserialize(response.bodyReader);
        if (list.isOk()) {
            this.#requestManager.resolveSuccess(response.frameId, list.unwrap().list);
        } else {
            this.#requestManager.resolveFailure(response.frameId, RpcStatus.Failed);
        }
    }
}
