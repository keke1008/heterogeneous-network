#pragma once

// IWYU pragma: begin_exports
#include "./serde/buffer.h"
#include "./serde/de.h"
#include "./serde/ser.h"
#include "./serde/span.h"

// IWYU pragma: end_exports

namespace nb {
    using de::DeserializeResult;
    using ser::SerializeResult;

    using de::AsyncBufferedReadable;
    using de::AsyncReadable;
    using ser::AsyncWritable;

    using de::AsyncDeserializable;
    using ser::AsyncSerializable;

    template <typename T>
    concept AsyncReadableWritable = AsyncReadable<T> && AsyncWritable<T>;
} // namespace nb
