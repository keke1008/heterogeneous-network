const checkUintBounds =
    (bits: number) =>
    (value: number): boolean => {
        const max = 2 ** bits;
        return value >= 0 && value < max;
    };

const assertUintBounds =
    (bits: number) =>
    (value: number): void => {
        if (!checkUintBounds(bits)(value)) {
            throw new Error(`Value ${value} is out of bounds for ${bits}-bit unsigned integer`);
        }
    };

const checkIntBounds =
    (bits: number) =>
    (value: number): boolean => {
        const max = 2 ** (bits - 1);
        return value >= -max && value < max;
    };

const assertIntBounds =
    (bits: number) =>
    (value: number): void => {
        if (!checkIntBounds(bits)(value)) {
            throw new Error(`Value ${value} is out of bounds for ${bits}-bit signed integer`);
        }
    };

export const bounds = {
    checkUint8: checkUintBounds(8),
    checkUint16: checkUintBounds(16),
    checkUint32: checkUintBounds(32),
    assertUint8: assertUintBounds(8),
    assertUint16: assertUintBounds(16),
    assertUint32: assertUintBounds(32),

    checkInt8: checkIntBounds(8),
    checkInt16: checkIntBounds(16),
    checkInt32: checkIntBounds(32),
    assertInt8: assertIntBounds(8),
    assertInt16: assertIntBounds(16),
    assertInt32: assertIntBounds(32),
};
