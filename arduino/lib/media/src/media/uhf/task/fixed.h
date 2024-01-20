#pragma once

#include "../response.h"

namespace media::uhf {
    enum class FixedTaskResult : uint8_t {
        Ok,
        Error,
    };

    template <
        nb::AsyncReadableWritable RW,
        typename Command,
        UhfResponseType ResponseType,
        uint8_t ResponseByteLength>
    class FixedTask {
        struct GetWritableLock {};

        template <nb::AsyncWritable W>
        class SendCommand {
            nb::LockGuard<etl::reference_wrapper<W>> writable_;

          public:
            explicit SendCommand(nb::LockGuard<etl::reference_wrapper<W>> &&writable)
                : writable_{etl::move(writable)} {}

            inline nb::Poll<nb::SerializeResult> execute(Command &command) {
                return command.serialize(writable_->get());
            }
        };

        struct WaitingForResponse {};

        template <nb::AsyncReadable R>
        struct ResponseReceiving {
            nb::LockGuard<etl::reference_wrapper<R>> readable;
            UhfFixedResponseBodyDeserializer<ResponseByteLength> response;

            explicit ResponseReceiving(nb::LockGuard<etl::reference_wrapper<R>> &&readable)
                : readable{etl::move(readable)} {}

            inline nb::Poll<nb::de::DeserializeResult> execute() {
                return response.deserialize(readable->get());
            }
        };

        Command command_;
        etl::variant<GetWritableLock, SendCommand<RW>, WaitingForResponse, ResponseReceiving<RW>>
            state_{GetWritableLock{}};

      public:
        FixedTask() = default;
        FixedTask(const FixedTask &) = delete;
        FixedTask(FixedTask &&) = default;
        FixedTask &operator=(const FixedTask &) = delete;
        FixedTask &operator=(FixedTask &&) = default;

        template <typename... Args>
        explicit FixedTask(Args &&...args) : command_{etl::forward<Args>(args)...} {}

        nb::Poll<FixedTaskResult> execute(nb::Lock<etl::reference_wrapper<RW>> &writable) {
            if (etl::holds_alternative<GetWritableLock>(state_)) {
                auto &&guard = POLL_MOVE_UNWRAP_OR_RETURN(writable.poll_lock());
                state_.template emplace<SendCommand<RW>>(etl::move(guard));
            }

            if (etl::holds_alternative<SendCommand<RW>>(state_)) {
                auto &state = etl::get<SendCommand<RW>>(state_);
                if (POLL_UNWRAP_OR_RETURN(state.execute(command_)) != nb::SerializeResult::Ok) {
                    return FixedTaskResult::Error;
                }
                state_.template emplace<WaitingForResponse>();
            }

            if (etl::holds_alternative<WaitingForResponse>(state_)) {
                return nb::pending;
            }

            if (etl::holds_alternative<ResponseReceiving<RW>>(state_)) {
                auto &state = etl::get<ResponseReceiving<RW>>(state_);
                return POLL_UNWRAP_OR_RETURN(state.execute()) == nb::DeserializeResult::Ok
                    ? FixedTaskResult::Ok
                    : FixedTaskResult::Error;
            }

            return nb::pending;
        }

        UhfHandleResponseResult handle_response(UhfResponse<RW> &&res) {
            if (etl::holds_alternative<WaitingForResponse>(state_) && res.type == ResponseType) {
                state_.template emplace<ResponseReceiving<RW>>(etl::move(res.body));
                return UhfHandleResponseResult::Handle;
            }

            return UhfHandleResponseResult::Invalid;
        }

        inline etl::string_view result() const {
            return etl::get<ResponseReceiving<RW>>(state_).response.result();
        }

        inline const etl::span<const uint8_t> span_result() const {
            return etl::get<ResponseReceiving<RW>>(state_).response.span_result();
        }
    };
}; // namespace media::uhf
