#pragma once

#include "./pool.h"
#include <nb/serde.h>
#include <nb/stream/fixed.h>

namespace net::frame {
    class FrameBufferReader final : public nb::stream::ReadableStream,
                                    public nb::stream::ReadableBuffer {
        FrameBufferReference buffer_ref_;
        nb::stream::FixedReadableBufferIndex index_;
        uint8_t offset_{0};

        FrameBufferReader(FrameBufferReference &&buffer_ref, uint8_t offset)
            : buffer_ref_{etl::move(buffer_ref)},
              offset_{offset} {}

        inline uint8_t written_count() const {
            uint8_t written = buffer_ref_.written_count();
            return written < offset_ ? 0 : written - offset_;
        }

        inline etl::span<uint8_t> span() const {
            return buffer_ref_.span().subspan(offset_);
        }

        inline uint8_t read_index() const {
            return index_.index();
        }

      public:
        FrameBufferReader() = delete;
        FrameBufferReader(const FrameBufferReader &) = delete;
        FrameBufferReader(FrameBufferReader &&) = default;
        FrameBufferReader &operator=(const FrameBufferReader &) = delete;
        FrameBufferReader &operator=(FrameBufferReader &&) = default;

        explicit FrameBufferReader(FrameBufferReference &&buffer_ref)
            : buffer_ref_{etl::move(buffer_ref)} {}

        inline uint8_t readable_count() const override {
            return index_.readable_count(written_count());
        }

        inline uint8_t read() override {
            return index_.read(span(), written_count());
        }

        inline void read(etl::span<uint8_t> buffer) override {
            index_.read(span(), written_count(), buffer);
        }

        template <nb::buf::IBufferParser Parser>
        inline decltype(auto) read() {
            return index_.read<Parser>(span(), written_count());
        }

        class AsyncBuffer {
            FrameBufferReader &reader_;

          public:
            explicit AsyncBuffer(FrameBufferReader &reader) : reader_{reader} {}

            inline etl::span<const uint8_t> span() const {
                return reader_.span().subspan(reader_.read_index(), reader_.readable_count());
            }
        };

        template <nb::buf::IAsyncParser<AsyncBuffer> Parser>
        inline decltype(auto) read(Parser &parser) {
            AsyncBuffer buf{*this};
            return index_.read<AsyncBuffer, Parser>(parser, buf);
        }

        class AsyncReadable {
            FrameBufferReader &reader_;

          public:
            explicit AsyncReadable(FrameBufferReader &reader) : reader_{reader} {}

            nb::Poll<nb::de::DeserializeResult> poll_readable(uint8_t read_count) {
                uint8_t required_index = reader_.read_index() + read_count;
                if (required_index > reader_.frame_length()) {
                    return nb::de::DeserializeResult::NotEnoughLength;
                }
                if (required_index > reader_.written_count()) {
                    return nb::pending;
                }
                return nb::de::DeserializeResult::Ok;
            }

            inline uint8_t read_unchecked() {
                return reader_.read();
            }

            inline nb::Poll<nb::de::DeserializeResult> read(uint8_t &dest) {
                SERDE_DESERIALIZE_OR_RETURN(poll_readable(1));
                dest = read_unchecked();
                return nb::de::DeserializeResult::Ok;
            }
        };

        template <nb::de::AsyncDeserializable<AsyncReadable> Deserializer>
        nb::Poll<nb::de::DeserializeResult> deserialize(Deserializer &deserializer) {
            AsyncReadable readable{*this};
            return deserializer.deserialize(readable);
        }

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return index_.read_all_into(span(), written_count(), destination);
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.frame_length() - offset_;
        }

        bool is_buffer_filled() const {
            return buffer_ref_.written_count() == buffer_ref_.frame_length();
        }

        inline bool is_all_read() const {
            return is_buffer_filled() && readable_count() == 0;
        }

        inline etl::span<const uint8_t> written_buffer() const {
            return span().subspan(0, written_count());
        }

        inline FrameBufferReader make_initial_clone() {
            return FrameBufferReader{buffer_ref_.clone()};
        }

        inline void reset() {
            index_.reset();
        }

        inline FrameBufferReader subreader() {
            return FrameBufferReader{buffer_ref_.clone(), read_index()};
        }
    };

    inline logger::log::Printer &
    operator<<(logger::log::Printer &printer, const FrameBufferReader &reader) {
        printer << '(' << reader.readable_count() << '/' << reader.frame_length() << ')';
        printer << reader.written_buffer();
        return printer;
    }

    class FrameBufferWriter final : public nb::stream::WritableStream,
                                    public nb::stream::WritableBuffer {
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

        inline uint8_t writable_count() const override {
            return buffer_ref_.write_index().writable_count(buffer_ref_.frame_length());
        }

        inline bool write(uint8_t byte) override {
            return buffer_ref_.write_index().write(
                buffer_ref_.span(), buffer_ref_.frame_length(), byte
            );
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return buffer_ref_.write_index().write(
                buffer_ref_.span(), buffer_ref_.frame_length(), buffer
            );
        }

        template <nb::buf::IBufferWriter... Writers>
        inline bool write(Writers &&...writers) {
            return buffer_ref_.write_index().write(
                buffer_ref_.span(), buffer_ref_.frame_length(), etl::forward<Writers>(writers)...
            );
        }

        class AsyncWritable {
            FrameBufferWriter &writer_;

          public:
            explicit AsyncWritable(FrameBufferWriter &writer) : writer_{writer} {}

            nb::Poll<nb::ser::SerializeResult> poll_writable(uint8_t write_count) {
                uint8_t required_index = writer_.buffer_ref_.write_index().index() + write_count;
                if (required_index > writer_.frame_length()) {
                    return nb::ser::SerializeResult::NotEnoughLength;
                }
                return nb::ser::SerializeResult::Ok;
            }

            inline void write_unchecked(uint8_t byte) {
                writer_.write(byte);
            }

            inline nb::Poll<nb::ser::SerializeResult> write(uint8_t byte) {
                SERDE_SERIALIZE_OR_RETURN(poll_writable(1));
                write_unchecked(byte);
                return nb::ser::SerializeResult::Ok;
            }
        };

        template <nb::ser::AsyncSerializable<AsyncWritable> Serializable>
        nb::Poll<nb::ser::SerializeResult> serialize(Serializable &serializable) {
            AsyncWritable writable{*this};
            return serializable.serialize(writable);
        }

        template <nb::ser::AsyncSerializable<AsyncWritable> Serializables>
        void serialize_all_at_once(Serializables &&serializable) {
            AsyncWritable writable{*this};
            auto poll = serializable.serialize(writable);
            ASSERT(poll.is_ready() && poll.unwrap() == nb::ser::SerializeResult::Ok);
        }

        inline nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return buffer_ref_.write_index().write_all_from(
                buffer_ref_.span(), buffer_ref_.frame_length(), source
            );
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.frame_length();
        }

        bool is_buffer_filled() const {
            return buffer_ref_.written_count() == buffer_ref_.frame_length();
        }

        inline void shrink_frame_length_to_fit() {
            buffer_ref_.shrink_frame_length_to_fit();
        }

        inline FrameBufferReader make_initial_reader() {
            return FrameBufferReader{buffer_ref_.clone()};
        }

        template <typename... Args>
        [[deprecated("use `write` instead")]] inline void build(Args &&...args) {
            write(etl::forward<Args>(args)...);
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
