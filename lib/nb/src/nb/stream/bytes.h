#pragma once

#include "stream.h"
#include <etl/optional.h>
#include <etl/utility.h>
#include <etl/vector.h>
#include <memory/shared.h>
#include <memory/track_ptr.h>
#include <stdint.h>

namespace nb::stream {
    template <size_t N>
    class BytesStream {
        memory::Shared<etl::vector<uint8_t, N>> buffer_;

      public:
        BytesStream() = default;
        BytesStream(const BytesStream &) = default;
        BytesStream &operator=(const BytesStream &) = default;
        BytesStream(BytesStream &&other) = default;
        BytesStream &operator=(BytesStream &&other) = default;

        inline size_t writable_count() {
            return buffer_->available();
        }

        inline size_t readable_count() {
            return buffer_.size();
        }

        inline bool write(uint8_t item) {
            if (buffer_.isFull()) {
                return false;
            }
            buffer_.push(item);
            return true;
        }

        inline etl::optional<uint8_t> read() {
            if (buffer_.isEmpty()) {
                return {};
            }
            return buffer_.pop();
        }
    };

    class Empty {};

    template <size_t N>
    class BytesStreamWriter {
        memory::TrackPtr<Empty, BytesStream<N>> stream_;

      public:
        using WritableStreamItem = uint8_t;

        BytesStreamWriter(memory::TrackPtr<Empty, BytesStream<N>> stream) : stream_(stream) {}

        inline size_t writable_count() {
            auto stream = stream_.try_get_pair();
            return stream ? stream->writable_count() : 0;
        }

        inline bool write(WritableStreamItem item) {
            auto stream = stream_.try_get_pair();
            return stream ? stream->write(item) : false;
        }

        inline bool is_closed() {
            return !stream_.try_get_pair();
        }
    };

    static_assert(is_stream_writer_v<BytesStreamWriter<0>, uint8_t>);

    template <size_t N>
    class BytesStreamReader {
        memory::TrackPtr<BytesStream<N>, Empty> stream_;

      public:
        using ReadableStreamItem = uint8_t;

        inline size_t readable_count() {
            auto stream = stream_.try_get_pair();
            return stream ? stream->readable_count() : 0;
        }

        inline etl::optional<uint8_t> read() {
            auto stream = stream_.try_get_pair();
            return stream ? stream->read() : false;
        }

        inline bool is_closed() {
            return !stream_.try_get_pair();
        }
    };

    static_assert(is_stream_reader_v<BytesStreamReader<0>, uint8_t>);

    template <size_t N>
    etl::pair<BytesStreamReader<N>, BytesStreamWriter<N>> make_bytes_stream() {
        auto [stream, empty] = memory::make_track_ptr<BytesStream<N>, Empty>();
        return {BytesStreamReader<N>(empty), BytesStreamWriter<N>(stream)};
    }
} // namespace nb::stream
