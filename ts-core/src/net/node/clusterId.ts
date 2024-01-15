import { UniqueKey } from "@core/object";
import { TransformSerdeable, Uint8Serdeable } from "@core/serde";
import * as z from "zod";

const NO_CLUSTER_ID = 0 as const;

export class NoCluster implements UniqueKey {
    static readonly serdeable = new TransformSerdeable(
        new Uint8Serdeable(),
        () => new NoCluster(),
        () => 0,
    );

    id(): number {
        return NO_CLUSTER_ID;
    }

    isNoCluster(): true {
        return true;
    }

    equals(other: ClusterId | NoCluster): boolean {
        return other instanceof NoCluster;
    }

    toHumanReadableString(): string {
        return "0";
    }

    toString(): string {
        return "NoCluster";
    }

    uniqueKey(): string {
        return "NoCluster";
    }

    display(): string {
        return "NoCluster";
    }

    toJSON(): string {
        return this.display();
    }
}

export class ClusterId implements UniqueKey {
    #id: number;

    private constructor(id: number) {
        if (id === NO_CLUSTER_ID) {
            throw new Error("Invalid cluster id");
        }
        this.#id = id;
    }

    static noCluster(): NoCluster {
        return new NoCluster();
    }

    isNoCluster(): false {
        return false;
    }

    id(): number {
        return this.#id;
    }

    static fromNumber(id: number): ClusterId | NoCluster {
        return id === NO_CLUSTER_ID ? ClusterId.noCluster() : new ClusterId(id);
    }

    static readonly serdeable = new TransformSerdeable(new Uint8Serdeable(), ClusterId.fromNumber, (id) => id.id());

    static noClusterExcludedSerdeable = new TransformSerdeable(
        new Uint8Serdeable(),
        (id) => (id === NO_CLUSTER_ID ? undefined : new ClusterId(id)),
        (id) => id.id(),
    );

    static schema = z
        .union([z.string().min(1), z.number()])
        .pipe(z.coerce.number().min(0).max(255))
        .transform(ClusterId.fromNumber);
    static noClusterExcludedSchema = z
        .union([z.string().min(1), z.number()])
        .pipe(z.coerce.number().min(1).max(255))
        .transform((v) => new ClusterId(v));

    equals(other: ClusterId | NoCluster): boolean {
        return other instanceof ClusterId && this.#id === other.#id;
    }

    toHumanReadableString(): string {
        return this.#id.toString();
    }

    toString(): string {
        return `ClusterId(${this.toHumanReadableString()})`;
    }

    uniqueKey(): string {
        return this.#id.toString();
    }

    display(): string {
        return this.toHumanReadableString();
    }

    toJSON(): string {
        return this.toHumanReadableString();
    }
}

export type OptionalClusterId = ClusterId | NoCluster;
