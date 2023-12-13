import { z } from "zod";
import { NetNs, IpAddress } from "./ip";
import { executeCommand } from "./common";

export class Table {
    type: "table" = "table" as const;
    #name: string;
    #netNs: NetNs;

    constructor(args: { name: string; netNs: NetNs }) {
        this.#name = args.name;
        this.#netNs = args.netNs;
    }

    get netNs(): NetNs {
        return this.#netNs;
    }

    toString(): string {
        return this.#name;
    }
}

export type ChainHook = "prerouting" | "postrouting";

export class Chain {
    type: "chain" = "chain" as const;
    #name: string;
    #table: Table;
    #hook: ChainHook;

    constructor(args: { name: string; table: Table; hook: ChainHook }) {
        this.#name = args.name;
        this.#table = args.table;
        this.#hook = args.hook;
    }

    get netNs(): NetNs {
        return this.#table.netNs;
    }

    get table(): Table {
        return this.#table;
    }

    get hook(): ChainHook {
        return this.#hook;
    }

    toString(): string {
        return this.#name;
    }
}

export class Port {
    #port: number;

    private constructor(args: { port: number }) {
        this.#port = args.port;
    }

    toString(): string {
        return this.#port.toString();
    }

    equals(other: Port): boolean {
        return this.#port === other.#port;
    }

    static readonly schema = z.coerce
        .number()
        .min(0)
        .max(65535)
        .transform((value) => new Port({ port: value }));

    toNumber(): number {
        return this.#port;
    }
}

export class RuleHandle {
    #chain: Chain;
    #handle: number;

    get chain(): Chain {
        return this.#chain;
    }

    constructor(args: { chain: Chain; handle: number }) {
        this.#chain = args.chain;
        this.#handle = args.handle;
    }

    toString(): string {
        return this.#handle.toString();
    }
}

export class SNatRule {
    #saddr: IpAddress;
    #sport: Port;
    #to: Port;

    get saddr(): IpAddress {
        return this.#saddr;
    }

    get sport(): Port {
        return this.#sport;
    }

    get to(): Port {
        return this.#to;
    }

    constructor(args: { saddr: IpAddress; sport: Port; to: Port }) {
        this.#saddr = args.saddr;
        this.#sport = args.sport;
        this.#to = args.to;
    }

    equals(other: object): boolean {
        return (
            other instanceof SNatRule &&
            this.#saddr.equals(other.#saddr) &&
            this.#sport.equals(other.#sport) &&
            this.#to.equals(other.#to)
        );
    }

    static readonly #exprSchema = z.union([
        z
            .object({
                match: z.object({
                    op: z.literal("=="),
                    left: z.object({ payload: z.object({ protocol: z.literal("ip"), field: z.literal("saddr") }) }),
                    right: IpAddress.schema,
                }),
            })
            .transform((value) => ({ saddr: value.match.right })),
        z
            .object({
                match: z.object({
                    op: z.literal("=="),
                    left: z.object({ payload: z.object({ protocol: z.literal("udp"), field: z.literal("sport") }) }),
                    right: Port.schema,
                }),
            })
            .transform((value) => ({ sport: value.match.right })),
        z
            .object({
                snat: z.object({ port: Port.schema }),
            })
            .transform((value) => ({ to: value.snat.port })),
    ]);
    static readonly schema = z.tuple([this.#exprSchema, this.#exprSchema, this.#exprSchema]).transform((exprs, ctx) => {
        const expr = { ...exprs[0], ...exprs[1], ...exprs[2] };
        if ("saddr" in expr && "sport" in expr && "to" in expr) {
            return new SNatRule({ saddr: expr.saddr, sport: expr.sport, to: expr.to });
        } else {
            ctx.addIssue({ code: z.ZodIssueCode.custom, message: "invalid snat rule" });
            return z.NEVER;
        }
    });
}

export class DNatRule {
    #dport: Port;
    #toaddr: IpAddress;
    #toport: Port;

    get dport(): Port {
        return this.#dport;
    }

    get toaddr(): IpAddress {
        return this.#toaddr;
    }

    get toport(): Port {
        return this.#toport;
    }

    constructor(args: { dport: Port; toaddr: IpAddress; toport: Port }) {
        this.#dport = args.dport;
        this.#toaddr = args.toaddr;
        this.#toport = args.toport;
    }

    equals(other: object): boolean {
        return (
            other instanceof DNatRule &&
            this.#dport.equals(other.#dport) &&
            this.#toaddr.equals(other.#toaddr) &&
            this.#toport.equals(other.#toport)
        );
    }

    static readonly #exprSchema = z.union([
        z
            .object({
                match: z.object({
                    op: z.literal("=="),
                    left: z.object({ payload: z.object({ protocol: z.literal("udp"), field: z.literal("dport") }) }),
                    right: Port.schema,
                }),
            })
            .transform((value) => ({ dport: value.match.right })),
        z
            .object({
                dnat: z.object({ addr: IpAddress.schema, port: Port.schema }),
            })
            .transform((value) => ({ toaddr: value.dnat.addr, toport: value.dnat.port })),
    ]);
    static readonly schema = z.tuple([this.#exprSchema, this.#exprSchema, this.#exprSchema]).transform((exprs, ctx) => {
        const expr = { ...exprs[0], ...exprs[1], ...exprs[2] };
        if ("dport" in expr && "toaddr" in expr && "toport" in expr) {
            return new DNatRule({ dport: expr.dport, toaddr: expr.toaddr, toport: expr.toport });
        } else {
            ctx.addIssue({ code: z.ZodIssueCode.custom, message: "invalid dnat rule" });
            return z.NEVER;
        }
    });
}

export class NatRule {
    #handle: RuleHandle;
    #chain: Chain;
    #natRule: SNatRule | DNatRule;

    get handle(): RuleHandle {
        return this.#handle;
    }

    get chain(): Chain {
        return this.#chain;
    }

    get natRule(): SNatRule | DNatRule {
        return this.#natRule;
    }

    static readonly schema = z.object({
        rule: z.object({
            family: z.literal("ip"),
            handle: z.number(),
            expr: z.union([SNatRule.schema, DNatRule.schema]),
        }),
    });

    private constructor(args: { handle: RuleHandle; chain: Chain; natRule: SNatRule | DNatRule }) {
        this.#handle = args.handle;
        this.#chain = args.chain;
        this.#natRule = args.natRule;
    }

    static #fromRuleJson(args: { chain: Chain; rulesJson: unknown }): NatRule {
        const { chain, rulesJson } = args;
        const root = this.schema.parse(rulesJson);
        const number = new RuleHandle({ chain, handle: root.rule.handle });
        return new NatRule({ handle: number, chain, natRule: root.rule.expr });
    }

    static fromJson(args: { chain: Chain; json: { nftables: Record<string, unknown>[] } }): NatRule[] {
        const { chain, json } = args;
        const rules = json.nftables.filter((row) => "rule" in row);
        return rules.map((rule) => this.#fromRuleJson({ chain, rulesJson: rule }));
    }
}

class RawNftablesCommand {
    async addTable(args: { netNs: NetNs; name: string }): Promise<Table> {
        const { netNs, name } = args;
        const command = `${netNs.commandPrefix()} nft add table ip ${name}`;
        await executeCommand(command);
        return new Table({ name, netNs });
    }

    async deleteTable(args: { table: Table }): Promise<void> {
        const { table } = args;
        const command = `${table.netNs.commandPrefix()} nft delete table ip ${table}`;
        await executeCommand(command);
    }

    async addNatChain(args: { table: Table; hook: ChainHook; name: string }): Promise<Chain> {
        const { table, hook, name } = args;
        const chainSpec = `{ type nat hook ${hook} priority 0\\; }`;
        const command = `${table.netNs.commandPrefix()} nft add chain ip ${table} ${name} ${chainSpec}`;
        await executeCommand(command);
        return new Chain({ name, table, hook });
    }

    async deleteChain(args: { chain: Chain }): Promise<void> {
        const { chain } = args;
        const command = `${chain.netNs.commandPrefix()} nft delete chain ip ${chain}`;
        await executeCommand(command);
    }

    async getRules(args: { chain: Chain }): Promise<NatRule[]> {
        const { chain } = args;
        const command = `${chain.netNs.commandPrefix()} nft --json list chain ip ${chain.table} ${chain}`;
        const stdout = await executeCommand(command);
        const rootSchema = z.object({ nftables: z.array(z.record(z.unknown())) });
        const root = rootSchema.parse(JSON.parse(stdout));
        return NatRule.fromJson({ chain, json: root });
    }

    async getSpecificRule(args: { chain: Chain; rule: SNatRule | DNatRule }): Promise<NatRule | undefined> {
        const { chain, rule } = args;
        const rules = await this.getRules({ chain });
        return rules.find((r) => r.natRule.equals(rule));
    }

    async addSNatRule(args: { chain: Chain; saddr: IpAddress; sport: Port; to: Port }): Promise<RuleHandle> {
        const snatRule = new SNatRule({ saddr: args.saddr, sport: args.sport, to: args.to });
        if (await this.getSpecificRule({ chain: args.chain, rule: snatRule })) {
            throw new Error("rule already exists");
        }

        const { chain, saddr, sport, to } = args;
        const snatOpts = `ip saddr ${saddr} udp sport ${sport} snat to :${to}`;
        const command = `${chain.netNs.commandPrefix()} nft add rule ip ${chain.table} ${chain} ${snatOpts}`;
        await executeCommand(command);

        const rule = await this.getSpecificRule({ chain, rule: snatRule });
        if (!rule) {
            throw new Error("rule not found");
        }
        return rule.handle;
    }

    async addDNatRule(args: { chain: Chain; dport: Port; toaddr: IpAddress; toport: Port }): Promise<RuleHandle> {
        const dnatRule = new DNatRule({ dport: args.dport, toaddr: args.toaddr, toport: args.toport });
        if (await this.getSpecificRule({ chain: args.chain, rule: dnatRule })) {
            throw new Error("rule already exists");
        }

        const { chain, dport, toaddr, toport } = args;
        const dnatOpts = `udp dport ${dport} dnat to ${toaddr}:${toport}`;
        const command = `${chain.netNs.commandPrefix()} nft add rule ip ${chain.table} ${chain} ${dnatOpts}`;
        await executeCommand(command);

        const rule = await this.getSpecificRule({ chain, rule: dnatRule });
        if (!rule) {
            throw new Error("rule not found");
        }
        return rule.handle;
    }

    async deleteRule(args: { handle: RuleHandle }): Promise<void> {
        const { handle } = args;
        const handleOpt = `ip ${handle.chain.table} ${handle.chain} handle ${handle}`;
        const command = `${handle.chain.netNs.commandPrefix()} nft delete rule ${handleOpt}`;
        await executeCommand(command);
    }
}

export class Transaction {
    #raw: RawNftablesCommand;
    #rollback: (() => Promise<void>)[] = [];

    constructor(args: { raw: RawNftablesCommand }) {
        this.#raw = args.raw;
    }

    async commit(): Promise<void> {
        this.#rollback = [];
    }

    async rollback(): Promise<void> {
        for (const rollback of this.#rollback) {
            try {
                await rollback();
            } catch (err) {
                console.warn(`[nft] error during rollback: ${err}`);
            }
        }
        this.#rollback = [];
    }

    async addTable(args: { netNs: NetNs; name: string }): Promise<Table> {
        const table = await this.#raw.addTable(args);
        this.#rollback.push(async () => await this.#raw.deleteTable({ table }));
        return table;
    }

    async deleteTable(args: { table: Table }): Promise<void> {
        await this.#raw.deleteTable(args);
        this.#rollback.push(async () => {
            await this.#raw.addTable({ netNs: args.table.netNs, name: args.table.toString() });
        });
    }

    async addNatChain(args: { table: Table; hook: ChainHook; name: string }): Promise<Chain> {
        const chain = await this.#raw.addNatChain(args);
        this.#rollback.push(async () => await this.#raw.deleteChain({ chain }));
        return chain;
    }

    async deleteChain(args: { chain: Chain }): Promise<void> {
        await this.#raw.deleteChain(args);
        this.#rollback.push(async () => {
            await this.#raw.addNatChain({
                table: args.chain.table,
                hook: args.chain.hook,
                name: args.chain.toString(),
            });
        });
    }

    async getRules(args: { chain: Chain }): Promise<NatRule[]> {
        return await this.#raw.getRules(args);
    }

    async hasRule(args: { chain: Chain; rule: SNatRule | DNatRule }): Promise<boolean> {
        const rules = await this.getRules(args);
        return rules.some((r) => r.natRule.equals(args.rule));
    }

    async addSNatRule(args: { chain: Chain; saddr: IpAddress; sport: Port; to: Port }): Promise<RuleHandle> {
        const handle = await this.#raw.addSNatRule(args);
        this.#rollback.push(async () => await this.#raw.deleteRule({ handle }));
        return handle;
    }

    async addDNatRule(args: { chain: Chain; dport: Port; toaddr: IpAddress; toport: Port }): Promise<RuleHandle> {
        const handle = await this.#raw.addDNatRule(args);
        this.#rollback.push(async () => await this.#raw.deleteRule({ handle }));
        return handle;
    }
}

export class NftablesCommand {
    #raw: RawNftablesCommand = new RawNftablesCommand();

    async withTransaction<T>(callback: (transaction: Transaction) => Promise<T>): Promise<T> {
        const tx = new Transaction({ raw: this.#raw });
        try {
            return await callback(tx);
        } catch (err) {
            console.warn(`[nft] transaction rollback: ${err}`);
            await tx.rollback();
            throw err;
        }
    }
}
