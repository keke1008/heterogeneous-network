import { EmptySerdeable, TransformSerdeable } from "@core/serde";
import { AddressType } from "./type";

export class LoopbackAddress {
    readonly type: AddressType.Loopback = AddressType.Loopback as const;

    static readonly serdeable = new TransformSerdeable(
        new EmptySerdeable(),
        () => new LoopbackAddress(),
        () => [],
    );

    static bodyBytesSize(): number {
        return 0;
    }

    body(): Uint8Array {
        return new Uint8Array();
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
