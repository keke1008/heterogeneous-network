#pragma once

#include "./serde/de.h"
#include "./serde/ser.h"
#include "./serde/span.h"

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
