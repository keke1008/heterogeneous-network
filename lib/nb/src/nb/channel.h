#pragma once

#include "./poll.h"
#include <etl/optional.h>
#include <memory/pair_shared.h>

namespace nb {
    template <typename T>
    class OneBufferSender {
        memory::Reference<etl::optional<T>> ref_;

      public:
        inline bool is_closed() const {
            return !ref_.has_pair();
        }

        inline nb::Poll<void> poll_closed() const {
            return is_closed() ? nb::ready() : nb::pending;
        }

        inline void close() && {
            ref_.unpair();
        }

        inline bool is_sendable() const {
            return !is_closed() && !ref_.get().has_value();
        }

        inline nb::Poll<void> poll_sendable() const {
            return is_sendable() ? nb::ready() : nb::pending;
        }

        /**
         * データを送信する．
         *
         * `ready`が返る場合にのみ`t`はmoveされる．
         */
        inline nb::Poll<void> send(T &&t) {
            POLL_UNWRAP_OR_RETURN(poll_sendable());
            ref_.get().value().get() = etl::move(t);
            return nb::ready();
        }
    };

    template <typename T>
    class OneBufferReceiver {
        memory::Owned<etl::optional<T>> buf_;

      public:
        inline bool is_closed() const {
            return !(buf_.get().has_value() || buf_.has_pair());
        }

        inline nb::Poll<void> poll_closed() const {
            return is_closed() ? nb::ready() : nb::pending;
        }

        inline void close() && {
            buf_.unpair();
        }

        inline bool is_receivable() const {
            return !is_closed() && buf_.get().has_value();
        }

        inline nb::Poll<void> poll_receivable() const {
            return is_receivable() ? nb::ready() : nb::pending;
        }

        inline nb::Poll<T> receive() {
            POLL_UNWRAP_OR_RETURN(poll_receivable());
            auto t = etl::move(buf_.get().value().get());
            buf_.get() = etl::nullopt;
            return nb::ready(etl::move(t));
        }

        inline nb::Poll<etl::reference_wrapper<const T>> peek() const {
            POLL_UNWRAP_OR_RETURN(poll_receivable());
            return nb::ready(etl::ref(buf_.get().value().get()));
        }

        inline nb::Poll<etl::reference_wrapper<T>> peek() {
            POLL_UNWRAP_OR_RETURN(poll_receivable());
            return nb::ready(etl::ref(buf_.get().value().get()));
        }
    };

    template <typename T>
    inline etl::pair<OneBufferSender<T>, OneBufferReceiver<T>> make_one_buffer_channel() {
        auto [owned, ref] = memory::make_shared<etl::optional<T>>();
        return etl::make_pair(
            OneBufferSender<T>{etl::move(ref)}, OneBufferReceiver<T>{etl::move(owned)}
        );
    }
} // namespace nb
