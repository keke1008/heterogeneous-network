import { ChecksumWriter } from "./checksum";

describe("checksum", () => {
    const data = new Uint8Array([0x01, 0x02, 0x03, 0x04]);
    const checksum = ~(0x0102 + 0x0304) & 0xffff;

    it("calculates checksum", () => {
        const writer = new ChecksumWriter();
        writer.writeBytes(data);
        expect(writer.checksum()).toBe(checksum);
    });

    it("verifies checksum", () => {
        const vdata = new Uint8Array([...data, checksum >> 8, checksum & 0xff]);
        const writer = new ChecksumWriter();
        writer.writeBytes(vdata);
        expect(writer.verify()).toBe(true);
    });
});
describe("checksum with no 16-bit word", () => {
    const data = new Uint8Array([0x01, 0x02, 0x03, 0x04, 0x05]);
    const sum = 0x0102 + 0x0304 + 0x0500;
    const checksum = ~(sum & 0xffff) & 0xffff;

    it("calculates checksum with no 16-bit word", () => {
        const writer = new ChecksumWriter();
        writer.writeBytes(data);
        expect(writer.checksum()).toBe(checksum);
    });

    it("verifies checksum with no 16-bit word", () => {
        const vdata = new Uint8Array([...data.slice(0, 4), checksum >> 8, checksum & 0xff, data[4]]);
        const writer = new ChecksumWriter();
        writer.writeBytes(vdata);
        expect(writer.verify()).toBe(true);
    });
});

describe("checksum with overflow", () => {
    const data = new Uint8Array([0x01, 0x02, 0x03, 0x04, 0xff, 0xff]);
    const sum = 0x0102 + 0x0304 + 0xffff;
    const checksum = ~((sum & 0xffff) + (sum >> 16)) & 0xffff;

    it("calculates checksum with overflow", () => {
        const writer = new ChecksumWriter();
        writer.writeBytes(data);
        expect(writer.checksum()).toBe(checksum);
    });

    it("verifies checksum with overflow", () => {
        const vdata = new Uint8Array([...data, checksum >> 8, checksum & 0xff]);
        const writer = new ChecksumWriter();
        writer.writeBytes(vdata);
        expect(writer.verify()).toBe(true);
    });
});
