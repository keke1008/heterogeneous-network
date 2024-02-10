#pragma once

#include <etl/span.h>
#include <etl/string_view.h>
#include <logger.h>
#include <nb/lock.h>
#include <nb/serde.h>
#include <util/span.h>

namespace media::wifi {
    enum class MessageType : uint8_t {
        // 末尾に\r\nがない不明なメッセージ
        UnknownHeader,

        // 末尾に\rがある不明なメッセージ
        UnknownHeaderEndWithCR,

        // 末尾に\r\nがある不明なメッセージ
        UnknownMessage,

        // "WIFI "で始まるメッセージ
        // 例: WIFI CONNECTED
        WifiHeader,

        // "+IPD,"で始まるメッセージ
        IPDHeader,

        // "OK\r\n"
        Ok,

        // "ERROR\r\n"
        Error,

        // "FAIL\r\n"
        Fail,

        // "SEND OK\r\n"
        SendOk,

        // "SEND FAIL\r\n"
        SendFail,

        // "> "
        // AT+CIPSENDのプロンプト
        SendPrompt,

    };

    namespace deserializer {
        inline bool is_buffer_complete(etl::span<const uint8_t> buffer) {
            uint8_t size = buffer.size();
            return size >= 2 && buffer[size - 2] == '\r' && buffer[size - 1] == '\n';
        }

        inline etl::optional<MessageType>
        try_deserialize_2_length_message(etl::span<const uint8_t> buffer) {
            if (buffer.size() != 2) {
                return etl::nullopt;
            }

            if (util::as_str(buffer) == "> ") {
                return MessageType::SendPrompt;
            } else {
                return etl::nullopt;
            }
        }

        inline etl::optional<MessageType>
        try_deserialize_5_length_message(etl::span<const uint8_t> buffer) {
            if (buffer.size() != 5) {
                return etl::nullopt;
            }

            if (util::as_str(buffer) == "WIFI ") {
                return MessageType::WifiHeader;
            } else if (util::as_str(buffer) == "+IPD,") {
                return MessageType::IPDHeader;
            } else {
                return etl::nullopt;
            }
        }

        inline etl::optional<MessageType>
        try_deserialize_complete_line_message(etl::span<const uint8_t> buffer) {
            if (!is_buffer_complete(buffer)) {
                return etl::nullopt;
            }

            auto body = util::as_str(buffer).substr(0, buffer.size() - 2);
            if (body == "OK") {
                return MessageType::Ok;
            } else if (body == "ERROR") {
                return MessageType::Error;
            } else if (body == "FAIL") {
                return MessageType::Fail;
            } else if (body == "SEND OK") {
                return MessageType::SendOk;
            } else if (body == "SEND FAIL") {
                return MessageType::SendFail;
            } else {
                return MessageType::UnknownMessage;
            }
        }

        template <uint8_t N>
        inline etl::optional<MessageType>
        try_deserialize_full_buffer_message(const etl::vector<uint8_t, N> buffer) {
            if (is_buffer_complete(buffer) || buffer.size() != N) {
                return etl::nullopt;
            }

            if (buffer.back() == '\r') {
                return MessageType::UnknownHeaderEndWithCR;
            } else {
                return MessageType::UnknownHeader;
            }
        }

        template <uint8_t N>
        etl::optional<MessageType> try_deserialize(const etl::vector<uint8_t, N> buffer) {
            etl::span<const uint8_t> buffer_span{buffer.data(), buffer.size()};

            if (auto result = try_deserialize_2_length_message(buffer_span)) {
                return result;
            } else if (auto result = try_deserialize_5_length_message(buffer_span)) {
                return result;
            } else if (auto result = try_deserialize_complete_line_message(buffer_span)) {
                return result;
            } else if (auto result = try_deserialize_full_buffer_message(buffer)) {
                return result;
            } else {
                return etl::nullopt;
            }
        }
    } // namespace deserializer

    template <nb::AsyncReadable R>
    struct EspATMessage {
        MessageType type;
        nb::LockGuard<etl::reference_wrapper<R>> body;
    };

    template <nb::AsyncReadable R>
    class MessageReceiver {
        static constexpr uint8_t MAX_KNOWN_MESSAGE_LENGTH = 11; // "SEND FAIL\r\n"の長さ
        etl::vector<uint8_t, MAX_KNOWN_MESSAGE_LENGTH> buffer_{};
        nb::LockGuard<etl::reference_wrapper<R>> readable_;

      public:
        MessageReceiver(nb::LockGuard<etl::reference_wrapper<R>> &&readable)
            : readable_{etl::move(readable)} {}

        nb::Poll<EspATMessage<R>> execute() {
            while (readable_->get().poll_readable(1).is_ready()) {
                buffer_.push_back(readable_.get().read_unchecked());

                if (auto type = deserializer::try_deserialize<MAX_KNOWN_MESSAGE_LENGTH>(buffer_)) {
                    return EspATMessage<R>{.type = *type, .body = etl::move(*readable_)};
                }
            }

            return nb::pending;
        }
    };
} // namespace media::wifi
