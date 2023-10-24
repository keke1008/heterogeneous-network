import { Sender } from "./channel";

describe("Sender", () => {
    it("should send a message", () => {
        const sender = new Sender<string>();
        sender.send("Hello World!");
    });

    it("throws when sending a message after close", () => {
        const sender = new Sender<string>();
        sender.close();
        expect(() => sender.send("Hello World!")).toThrow();
    });
});

describe("Receiver", () => {
    it("should receive a message", async () => {
        const sender = new Sender<string>();
        const receiver = sender.receiver();
        sender.send("Hello World!");
        expect(await receiver.next()).toStrictEqual({ done: false, value: "Hello World!" });
    });

    it("should receive a done message after close", async () => {
        const sender = new Sender<string>();
        const receiver = sender.receiver();
        sender.close();
        expect(await receiver.next()).toStrictEqual({ done: true, value: undefined });
    });

    it("maps messages", async () => {
        const sender = new Sender<number[]>();
        const receiver = sender.receiver().map((message) => message.length);
        sender.send([1, 2, 3]);
        expect(await receiver.next()).toStrictEqual({ done: false, value: 3 });
    });

    it("filters messages", async () => {
        const sender = new Sender<number>();
        const receiver = sender.receiver().filter((message) => message % 2 === 0);
        sender.send(1);
        sender.send(2);
        sender.send(3);
        sender.close();

        expect(await receiver.next()).toStrictEqual({ done: false, value: 2 });
        expect(await receiver.next()).toStrictEqual({ done: true, value: undefined });
    });

    it("filterMap messages", async () => {
        const sender = new Sender<number>();
        const receiver = sender.receiver().filterMap((message) => (message % 2 === 0 ? message * 2 : undefined));
        sender.send(1);
        sender.send(2);
        sender.send(3);
        sender.close();

        expect(await receiver.next()).toStrictEqual({ done: false, value: 4 });
        expect(await receiver.next()).toStrictEqual({ done: true, value: undefined });
    });

    it("forEach messages", async () => {
        const sender = new Sender<number>();
        const receiver = sender.receiver();
        sender.send(1);
        sender.send(2);
        sender.send(3);
        sender.close();

        const messages: number[] = [];
        await receiver.forEach((message) => messages.push(message));
        expect(messages).toStrictEqual([1, 2, 3]);
    });

    it("pipeTo messages", async () => {
        const sender = new Sender<number>();
        const receiver = sender.receiver();
        sender.send(1);
        sender.send(2);
        sender.send(3);
        sender.close();

        const sender2 = new Sender<number>();
        const receiver2 = sender2.receiver();
        await receiver.pipeTo(sender2);
        sender2.close();

        expect(await receiver2.next()).toStrictEqual({ done: false, value: 1 });
        expect(await receiver2.next()).toStrictEqual({ done: false, value: 2 });
        expect(await receiver2.next()).toStrictEqual({ done: false, value: 3 });
        expect(await receiver2.next()).toStrictEqual({ done: true, value: undefined });
    });
});
