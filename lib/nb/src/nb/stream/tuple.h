#pragma once

#include "./batch.h"
#include "./empty.h"
#include "./stream.h"
#include "./types.h"
#include <etl/functional.h>
#include <util/tuple.h>

namespace nb::stream {
    template <typename... Rs>
    class TupleReadableBuffer final : public ReadableBuffer {
        static_assert(
            (etl::is_convertible_v<Rs &, ReadableBuffer &> && ...),
            "All types must be convertible to ReadableBuffer"
        );

        util::Tuple<Rs...> buffers_;

      public:
        TupleReadableBuffer(Rs &&...buffers) : buffers_{etl::forward<Rs>(buffers)...} {}

        nb::Poll<void> read_all_into(WritableStream &destination) override {
            return util::apply(
                [&destination](auto &&...bs) {
                    bool done = (bs.read_all_into(destination).is_ready() && ...);
                    return done ? nb::ready() : nb::pending;
                },
                buffers_
            );
        }
    };

    template <typename... Ws>
    class TupleWritableBuffer final : public WritableBuffer {
        static_assert(
            (etl::is_convertible_v<Ws &, WritableBuffer &> && ...),
            "All types must be convertible to WritableBuffer"
        );

        util::Tuple<Ws...> buffers_;

      public:
        TupleWritableBuffer(Ws &&...buffers) : buffers_{etl::forward<Ws>(buffers)...} {}

        nb::Poll<void> write_all_from(ReadableStream &source) override {
            return util::apply(
                [&source](auto &&...bs) {
                    bool done = (bs.write_all_from(source).is_ready() && ...);
                    return done ? nb::ready() : nb::pending;
                },
                buffers_
            );
        }

        template <size_t I>
        auto &get() {
            return util::get<I>(buffers_);
        }

        template <size_t I>
        const auto &get() const {
            return util::get<I>(buffers_);
        }
    };
} // namespace nb::stream
