#pragma once

#include "./address.h"
#include <debug_assert.h>
#include <nb/future.h>
#include <nb/stream.h>

namespace net::link {
    class DataWriter final : public nb::stream::WritableStream {
        etl::reference_wrapper<nb::stream::WritableStream> stream_;
        nb::Promise<void> barrier_;
        uint8_t remain_frame_length_;

      public:
        explicit DataWriter(
            etl::reference_wrapper<nb::stream::ReadableWritableStream> stream,
            nb::Promise<void> &&barrier,
            uint8_t length
        )
            : stream_{stream},
              barrier_{etl::move(barrier)},
              remain_frame_length_{length} {}

        inline uint8_t writable_count() const override {
            return etl::min(remain_frame_length_, stream_.get().writable_count());
        }

        inline bool write(uint8_t byte) override {
            DEBUG_ASSERT(remain_frame_length_ > 0, "frame length is zero");
            remain_frame_length_--;
            return stream_.get().write(byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            DEBUG_ASSERT(remain_frame_length_ >= buffer.size(), "frame length is too short");
            remain_frame_length_ -= buffer.size();
            return stream_.get().write(buffer);
        }

        inline void close() {
            DEBUG_ASSERT(remain_frame_length_ == 0, "written bytes is not enough");
            barrier_.set_value();
        }
    };

    class DataReader final : public nb::stream::ReadableStream {
        etl::reference_wrapper<nb::stream::ReadableStream> stream_;
        uint8_t total_length_;
        nb::Promise<void> barrier_;
        uint8_t remain_frame_length_;

      public:
        explicit DataReader(
            etl::reference_wrapper<nb::stream::ReadableWritableStream> stream,
            nb::Promise<void> &&barrier,
            uint8_t length
        )
            : stream_{stream},
              total_length_{length},
              barrier_{etl::move(barrier)},
              remain_frame_length_{length} {}

        inline uint8_t readable_count() const override {
            return etl::min(remain_frame_length_, stream_.get().readable_count());
        }

        inline uint8_t read() override {
            DEBUG_ASSERT(remain_frame_length_ > 0, "frame length is zero");
            remain_frame_length_--;
            return stream_.get().read();
        }

        inline void read(etl::span<uint8_t> buffer) override {
            DEBUG_ASSERT(remain_frame_length_ >= buffer.size(), "frame length is too short");
            remain_frame_length_ -= buffer.size();
            return stream_.get().read(buffer);
        }

        inline void close() {
            DEBUG_ASSERT(remain_frame_length_ == 0, "read bytes is not enough");
            barrier_.set_value();
        }

        inline uint8_t total_length() const {
            return total_length_;
        }
    };

    struct FrameTransmission {
        nb::Future<DataWriter> body;

        /**
         * フレームの送信が成功したかどうかを表す．（相手に届いたかどうかではない）
         * 成功下かどうかの取得にメディアが対応していない場合は，常にtrueを返す．
         */
        nb::Future<bool> success;

        static util::Tuple<FrameTransmission, nb::Promise<DataWriter>, nb::Promise<bool>>
        make_frame_transmission() {
            auto [f_body, p_body] = nb::make_future_promise_pair<DataWriter>();
            auto [f_success, p_success] = nb::make_future_promise_pair<bool>();
            return util::Tuple{
                FrameTransmission{etl::move(f_body), etl::move(f_success)}, etl::move(p_body),
                etl::move(p_success)};
        }
    };

    struct FrameReception {
        nb::Future<DataReader> body;

        /**
         * フレームの送信元アドレス．
         * UHFの場合，送信元アドレスはフレームのボディの後に含まれるため，Futureで表現する必要がある．
         */
        nb::Future<Address> source;

        static util::Tuple<FrameReception, nb::Promise<DataReader>, nb::Promise<Address>>
        make_frame_reception() {
            auto [f_body, p_body] = nb::make_future_promise_pair<DataReader>();
            auto [f_source, p_source] = nb::make_future_promise_pair<Address>();
            return util::Tuple{
                FrameReception{etl::move(f_body), etl::move(f_source)}, etl::move(p_body),
                etl::move(p_source)};
        }
    };
} // namespace net::link
