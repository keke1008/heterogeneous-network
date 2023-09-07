#pragma once

#include <etl/functional.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <nb/poll.h>
#include <util/tuple.h>

namespace nb::stream {
    class StreamReader {
      public:
        using Item = uint8_t;

        virtual nb::Poll<Item> read() = 0;
        virtual nb::Poll<nb::Empty> poll() = 0;
    };

    template <typename T>
    class StreamWriter {
      public:
        using Item = uint8_t;

      protected:
        virtual nb::Poll<etl::reference_wrapper<Item>> write_reference() = 0;

      public:
        virtual nb::Poll<T> poll() = 0;

        nb::Poll<T> write(const Item data) {
            POLL_UNWRAP_OR_RETURN(write_reference()).get() = data;
            return poll();
        }

        nb::Poll<T> write(StreamReader &reader) {
            for (auto ref = write_reference(); ref.is_ready(); ref = write_reference()) {
                ref.unwrap().get() = POLL_UNWRAP_OR_RETURN(reader.read());
            }
            return poll();
        }

        nb::Poll<T> write(etl::span<Item> span) {
            for (auto &item : span) {
                POLL_UNWRAP_OR_RETURN(write_reference()).get() = item;
            }
            return poll();
        }
    };
} // namespace nb::stream
