import { EmptySerdeable, TransformSerdeable } from "@core/serde";
import { AddressType } from "./type";

export class LoopbackAddress {
    readonly type: AddressType.Loopback = AddressType.Loopback as const;

    static readonly serdeable = new TransformSerdeable(
        new EmptySerdeable(),
        () => new LoopbackAddress(),
        () => [],
    );

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
