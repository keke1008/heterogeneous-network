import { ConstantSerdeable } from "@core/serde";
import { AddressType } from "./type";
import { UniqueKey } from "@core/object";

export class LoopbackAddress implements UniqueKey {
    readonly type: AddressType.Loopback = AddressType.Loopback as const;

    static readonly serdeable = new ConstantSerdeable(new LoopbackAddress());

    static bodyBytesSize(): number {
        return 0;
    }

    body(): Uint8Array {
        return new Uint8Array();
    }

    equals(): boolean {
        return true;
    }

    uniqueKey(): string {
        return `${this.type}()`;
    }

    toString(): string {
        return "LoopbackAddress()";
    }
}
