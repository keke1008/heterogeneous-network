#pragma once

#include "./pool.h"
#include <nb/serde.h>

namespace net::frame {
    class FrameBufferReader {
        FrameBufferReference buffer_ref_;
        uint8_t read_index_{0};

      public:
        FrameBufferReader() = delete;
        FrameBufferReader(const FrameBufferReader &) = delete;
        FrameBufferReader(FrameBufferReader &&) = default;
        FrameBufferReader &operator=(const FrameBufferReader &) = delete;
        FrameBufferReader &operator=(FrameBufferReader &&) = default;

        explicit FrameBufferReader(FrameBufferReference &&buffer_ref)
            : buffer_ref_{etl::move(buffer_ref)} {}

        inline nb::Poll<nb::de::DeserializeResult> poll_readable(uint8_t read_count) {
            uint8_t required_index = read_index_ + read_count;
            if (required_index > buffer_ref_.buffer_length()) {
                return nb::de::DeserializeResult::NotEnoughLength;
            }
            if (required_index > buffer_ref_.written_index()) {
                return nb::pending;
            }
            return nb::de::DeserializeResult::Ok;
        }

        inline uint8_t read_unchecked() {
            return buffer_ref_.written_buffer()[read_index_++];
        }

        inline nb::Poll<nb::de::DeserializeResult> read(uint8_t &dest) {
            SERDE_DESERIALIZE_OR_RETURN(poll_readable(1));
            dest = read_unchecked();
            return nb::de::DeserializeResult::Ok;
        }

        template <nb::AsyncDeserializable<FrameBufferReader> Deserializer>
        nb::Poll<nb::de::DeserializeResult> deserialize(Deserializer &deserializer) {
            return deserializer.deserialize(*this);
        }

        template <nb::AsyncDeserializable<FrameBufferReader> Deserializers>
        void deserialize_all_at_once(Deserializers &&deserializers) {
            auto poll = deserializers.deserialize(*this);
            ASSERT(poll.is_ready() && poll.unwrap() == nb::de::DeserializeResult::Ok);
        }

        inline uint8_t buffer_length() const {
            return buffer_ref_.buffer_length();
        }

        inline bool is_all_written() const {
            return buffer_ref_.is_all_written();
        }

        inline bool is_all_read() const {
            return is_all_written() && read_index_ == buffer_length();
        }

        inline FrameBufferReader make_initial_clone() const {
            return FrameBufferReader{buffer_ref_.clone()};
        }

        inline FrameBufferReader subreader() const {
            return FrameBufferReader{buffer_ref_.slice(read_index_)};
        }

        inline FrameBufferReader origin() const {
            return FrameBufferReader{buffer_ref_.origin()};
        }

        friend inline logger::log::Printer &
        operator<<(logger::log::Printer &printer, const FrameBufferReader &reader) {
            printer << '(' << reader.read_index_;
            printer << '/' << reader.buffer_ref_.written_index();
            printer << '/' << reader.buffer_length() << ')';
            return printer << reader.buffer_ref_.written_buffer();
        }
    };

    class AsyncFrameBufferReaderSerializer {
        FrameBufferReader reader_;

      public:
        explicit AsyncFrameBufferReaderSerializer(FrameBufferReader &&reader)
            : reader_{etl::move(reader.origin())} {}

        template <nb::ser::AsyncWritable Writable>
        nb::Poll<nb::ser::SerializeResult> serialize(Writable &writable) {
            while (!reader_.is_all_read()) {
                POLL_UNWRAP_OR_RETURN(reader_.poll_readable(1));
                SERDE_SERIALIZE_OR_RETURN(writable.poll_writable(1));
                writable.write_unchecked(reader_.read_unchecked());
            }
            return nb::ser::SerializeResult::Ok;
        }

        inline uint8_t serialized_length() const {
            return reader_.buffer_length();
        }
    };

    class FrameBufferWriter {
        FrameBufferReference buffer_ref_;

      public:
        FrameBufferWriter() = delete;
        FrameBufferWriter(const FrameBufferWriter &) = delete;
        FrameBufferWriter(FrameBufferWriter &&) = default;
        FrameBufferWriter &operator=(const FrameBufferWriter &) = delete;
        FrameBufferWriter &operator=(FrameBufferWriter &&) = default;

        FrameBufferWriter(FrameBufferReference &&buffer_ref) : buffer_ref_{etl::move(buffer_ref)} {}

        static FrameBufferWriter empty() {
            return FrameBufferWriter{FrameBufferReference::empty()};
        }

        inline nb::Poll<nb::ser::SerializeResult> poll_writable(uint8_t write_count) {
            uint8_t required_index = buffer_ref_.written_index() + write_count;
            return required_index > buffer_ref_.buffer_length()
                ? nb::ser::SerializeResult::NotEnoughLength
                : nb::ser::SerializeResult::Ok;
        }

        inline void write_unchecked(uint8_t byte) {
            buffer_ref_.write_unchecked(byte);
        }

        inline nb::Poll<nb::ser::SerializeResult> write(uint8_t byte) {
            SERDE_SERIALIZE_OR_RETURN(poll_writable(1));
            write_unchecked(byte);
            return nb::ser::SerializeResult::Ok;
        }

        template <nb::ser::AsyncSerializable<FrameBufferWriter> Serializable>
        nb::Poll<nb::ser::SerializeResult> serialize(Serializable &serializable) {
            return serializable.serialize(*this);
        }

        template <nb::ser::AsyncSerializable<FrameBufferWriter> Serializables>
        void serialize_all_at_once(Serializables &&serializable) {
            auto poll = serializable.serialize(*this);
            ASSERT(poll.is_ready() && poll.unwrap() == nb::ser::SerializeResult::Ok);
        }

        inline bool is_all_written() const {
            return buffer_ref_.written_index() == buffer_ref_.buffer_length();
        }

        inline FrameBufferReader create_reader() const {
            return FrameBufferReader{buffer_ref_.clone()};
        }

        inline FrameBufferWriter subwriter() const {
            return FrameBufferWriter{buffer_ref_.slice(buffer_ref_.written_index())};
        }

        friend inline logger::log::Printer &
        operator<<(logger::log::Printer &printer, const FrameBufferWriter &writer) {
            printer << '(' << writer.buffer_ref_.written_index();
            printer << '/' << writer.buffer_ref_.buffer_length() << ')';
            return printer << writer.buffer_ref_.written_buffer();
        }
    };

    class AsyncFrameBufferWriterDeserializer {
        FrameBufferWriter writer_;

      public:
        explicit AsyncFrameBufferWriterDeserializer(FrameBufferWriter &&writer)
            : writer_{etl::move(writer)} {}

        template <nb::de::AsyncReadable Readable>
        nb::Poll<nb::de::DeserializeResult> deserialize(Readable &readable) {
            while (!writer_.is_all_written()) {
                SERDE_DESERIALIZE_OR_RETURN(readable.poll_readable(1));
                writer_.write_unchecked(readable.read_unchecked());
            }
            return nb::de::DeserializeResult::Ok;
        }
    };

    inline etl::pair<FrameBufferReader, FrameBufferWriter>
    make_frame_buffer_pair(FrameBufferReference &&buffer_ref) {
        return etl::pair{
            FrameBufferReader{etl::move(buffer_ref.clone())},
            FrameBufferWriter{etl::move(buffer_ref)},
        };
    }
} // namespace net::frame
