import { DeserializeResult } from "@core/serde";
import { Ok } from "oxide.ts";
import { AddressType } from "./type";

export class LoopbackAddress {
    readonly type: AddressType.Loopback = AddressType.Loopback as const;

    static deserialize(): DeserializeResult<LoopbackAddress> {
        return Ok(new LoopbackAddress());
    }

    serialize(): void {}

    serializedLength(): number {
        return 0;
    }

    equals(): boolean {
        return true;
    }

    toString(): string {
        return `${this.type}()`;
    }

    display(): string {
        return "LoopbackAddress()";
    }
}
