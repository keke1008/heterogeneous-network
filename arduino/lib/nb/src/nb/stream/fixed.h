#pragma once

#include "./types.h"
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/functional.h>
#include <etl/initializer_list.h>
#include <etl/span.h>
#include <logger.h>
#include <nb/buf.h>
#include <util/progmem.h>

namespace nb::stream {
    class FixedReadableBufferIndex {
        uint8_t read_index_{0};

      public:
        inline uint8_t index() const {
            return read_index_;
        }

        inline uint8_t readable_count(uint8_t readable_length) const {
            return readable_length - read_index_;
        }

        inline uint8_t read(etl::span<const uint8_t> bytes, uint8_t readable_length) {
            return bytes[read_index_++];
        }

        void
        read(etl::span<const uint8_t> bytes, uint8_t readable_length, etl::span<uint8_t> buffer) {
            uint8_t buffer_size = static_cast<uint8_t>(buffer.size());
            uint8_t read_count = etl::min(readable_count(readable_length), buffer_size);
            etl::copy_n(bytes.data() + read_index_, read_count, buffer.begin());
            read_index_ += read_count;
        }

        template <nb::buf::IBufferParser Parser>
        decltype(auto) read(etl::span<const uint8_t> bytes, uint8_t readable_length) {
            nb::buf::BufferSplitter splitter{bytes.subspan(read_index_)};
            decltype(auto) result = splitter.parse<Parser>();
            read_index_ += splitter.splitted_count();
            return result;
        }

        template <buf::IAsyncBuffer Buffer, buf::IAsyncParser<Buffer> Parser>
        nb::Poll<void> read(Parser &parser, Buffer &buffer) {
            buf::AsyncBufferSplitter<Buffer> splitter{buffer};
            POLL_UNWRAP_OR_RETURN(parser.template parse(splitter));
            read_index_ += splitter.splitted_count();
            return nb::ready();
        }

        nb::Poll<void> read_all_into(
            etl::span<const uint8_t> bytes,
            uint8_t readable_length,
            WritableStream &destination
        ) {
            bool continue_ = true;
            while (continue_) {
                uint8_t readable_count_ = readable_count(readable_length);
                uint8_t writable_count = destination.writable_count();
                uint8_t write_count = etl::min(readable_count_, writable_count);
                if (write_count == 0) {
                    break;
                }

                auto span = bytes.subspan(read_index_, write_count);
                continue_ = destination.write(span);
                read_index_ += write_count;
            }
            return read_index_ < readable_length ? nb::pending : nb::ready();
        }

        void reset() {
            read_index_ = 0;
        }
    };

    class FixedWritableBufferIndex {
        uint8_t index_{0};

      public:
        inline uint8_t index() const {
            return index_;
        }

        inline bool is_writable(uint8_t writable_length) const {
            return index_ < writable_length;
        }

        inline uint8_t writable_count(uint8_t writable_length) const {
            return writable_length - index_;
        }

        inline bool write(etl::span<uint8_t> bytes, uint8_t writable_length, uint8_t byte) {
            bytes[index_++] = byte;
            return is_writable(writable_length);
        }

        bool
        write(etl::span<uint8_t> bytes, uint8_t writable_length, etl::span<const uint8_t> buffer) {
            uint8_t buffer_size = static_cast<uint8_t>(buffer.size());
            uint8_t write_count = etl::min(writable_count(writable_length), buffer_size);
            etl::copy_n(buffer.begin(), write_count, bytes.data() + index_);
            index_ += write_count;
            return is_writable(writable_length);
        }

        template <nb::buf::IBufferWriter... Writers>
        bool write(etl::span<uint8_t> bytes, uint8_t writable_length, Writers &&...writers) {
            auto written_span = nb::buf::build_buffer(
                etl::span(bytes.data() + index_, writable_count(writable_length)),
                etl::forward<Writers>(writers)...
            );
            index_ += written_span.size();
            return is_writable(writable_length);
        }

        nb::Poll<void>
        write_all_from(etl::span<uint8_t> bytes, uint8_t writable_length, ReadableStream &source) {
            uint8_t read_count = etl::min(writable_count(writable_length), source.readable_count());
            source.read(etl::span(bytes.data() + index_, read_count));
            index_ += read_count;
            return is_writable(writable_length) ? nb::pending : nb::ready();
        }

        etl::span<uint8_t> written_bytes(etl::span<uint8_t> bytes) {
            return {bytes.data(), index_};
        }

        void reset() {
            index_ = 0;
        }
    };

    template <uint8_t MAX_LENGTH>
    class FixedReadableBuffer final : public ReadableStream, public ReadableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        FixedReadableBufferIndex index_;

      public:
        FixedReadableBuffer() = delete;

        template <typename... Rs>
        FixedReadableBuffer(Rs &&...rs) {
            length_ =
                nb::buf::build_buffer<Rs...>(etl::span(bytes_), etl::forward<Rs>(rs)...).size();
        }

        inline uint8_t readable_count() const override {
            return index_.readable_count(length_);
        }

        inline uint8_t read() override {
            return index_.read(bytes_, length_);
        }

        inline void read(etl::span<uint8_t> buffer) override {
            index_.read(bytes_, length_, buffer);
        }

        template <nb::buf::IBufferParser Parser>
        inline decltype(auto) read() {
            return index_.read<Parser>(bytes_, length_);
        }

        nb::Poll<void> read_all_into(WritableStream &destination) override {
            return index_.read_all_into(bytes_, length_, destination);
        }

        void reset() {
            index_.reset();
        }
    };

    template <uint8_t MAX_LENGTH>
    class FixedWritableBuffer final : public WritableStream, public WritableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        FixedWritableBufferIndex index_;

      public:
        FixedWritableBuffer() : length_{MAX_LENGTH} {};

        FixedWritableBuffer(uint8_t length) : length_{length} {};

        inline uint8_t writable_count() const override {
            return index_.writable_count(length_);
        }

        inline bool write(uint8_t byte) override {
            return index_.write(bytes_, length_, byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return index_.write(bytes_, length_, buffer);
        }

        inline nb::Poll<void> write_all_from(ReadableStream &source) override {
            return index_.write_all_from(bytes_, length_, source);
        }

        inline etl::span<uint8_t> written_bytes() {
            return index_.written_bytes(bytes_);
        }

        nb::Poll<etl::span<uint8_t>> poll() {
            return index_.is_writable(length_)
                ? nb::pending
                : nb::ready(etl::span(bytes_.data(), index_.index()));
        }

        void reset() {
            index_.reset();
        }
    };

    class IFixedReadableWritableBuffer : public ReadableWritableStream,
                                         public ReadableWritableBuffer {
        etl::span<uint8_t> bytes_;
        FixedReadableBufferIndex read_index_;
        FixedWritableBufferIndex write_index_;

      public:
        IFixedReadableWritableBuffer() = delete;
        IFixedReadableWritableBuffer(const IFixedReadableWritableBuffer &) = delete;
        IFixedReadableWritableBuffer(IFixedReadableWritableBuffer &&) = default;
        IFixedReadableWritableBuffer &operator=(const IFixedReadableWritableBuffer &) = delete;
        IFixedReadableWritableBuffer &operator=(IFixedReadableWritableBuffer &&) = default;

        IFixedReadableWritableBuffer(etl::span<uint8_t> bytes) : bytes_{bytes} {};

        inline uint8_t readable_count() const override {
            return read_index_.readable_count(write_index_.index());
        }

        inline uint8_t read() override {
            return read_index_.read(bytes_, write_index_.index());
        }

        inline void read(etl::span<uint8_t> buffer) override {
            read_index_.read(bytes_, write_index_.index(), buffer);
        }

        inline nb::Poll<void> read_all_into(WritableStream &destination) override {
            return read_index_.read_all_into(bytes_, write_index_.index(), destination);
        }

        inline uint8_t writable_count() const override {
            return write_index_.writable_count(bytes_.size());
        }

        inline bool write(uint8_t byte) override {
            return write_index_.write(bytes_, bytes_.size(), byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return write_index_.write(bytes_, bytes_.size(), buffer);
        }

        inline nb::Poll<void> write_all_from(ReadableStream &source) override {
            return write_index_.write_all_from(bytes_, bytes_.size(), source);
        }

        inline void reset() {
            read_index_.reset();
            write_index_.reset();
        }

        inline uint8_t length() const {
            return bytes_.size();
        }

        inline etl::span<uint8_t> written_bytes() {
            return write_index_.written_bytes(bytes_);
        }

      protected:
        inline void set_span(etl::span<uint8_t> bytes) {
            ASSERT(bytes.size() == bytes_.size());
            bytes_ = bytes;
        }
    };

    template <uint8_t MAX_LENGTH>
    class FixedReadableWritableBuffer final : public IFixedReadableWritableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;

      public:
        FixedReadableWritableBuffer() : IFixedReadableWritableBuffer{bytes_} {};
        FixedReadableWritableBuffer(const FixedReadableWritableBuffer &) = delete;

        FixedReadableWritableBuffer(FixedReadableWritableBuffer &&other)
            : IFixedReadableWritableBuffer{etl::move(other)},
              bytes_{etl::move(other.bytes_)} {
            set_span(bytes_);
        }

        FixedReadableWritableBuffer &operator=(const FixedReadableWritableBuffer &) = delete;

        FixedReadableWritableBuffer &operator=(FixedReadableWritableBuffer &&other) {
            IFixedReadableWritableBuffer::operator=(etl::move(other));
            bytes_ = etl::move(other.bytes_);
            set_span(bytes_);
            return *this;
        }

        FixedReadableWritableBuffer(uint8_t length)
            : IFixedReadableWritableBuffer{etl::span(bytes_.data(), length)} {};
    };

    template <uint8_t... Bytes>
    class ConstantReadableBuffer final : public ReadableBuffer, ReadableStream {

        static constexpr uint8_t SIZE = sizeof...(Bytes);
        PROGMEM static constexpr etl::array<uint8_t, SIZE> bytes_ = {Bytes...};

        FixedReadableBufferIndex index_;

      public:
        inline uint8_t readable_count() const override {
            return index_.readable_count(SIZE);
        }

        inline uint8_t read() override {
            return index_.read(bytes_, SIZE);
        }

        inline void read(etl::span<uint8_t> buffer) override {
            index_.read(bytes_, SIZE, buffer);
        }

        inline nb::Poll<void> read_all_into(WritableStream &destination) override {
            return index_.read_all_into(bytes_, SIZE, destination);
        }
    };
} // namespace nb::stream
