#pragma once

#include <nb/stream.h>

namespace media::uhf::modem {
    template <typename Command, typename Response>
    class Task {
        static_assert(nb::stream::is_stream_reader_v<nb::stream::StreamReaderDelegate<Command>>);
        static_assert(nb::stream::is_stream_writer_v<nb::stream::StreamWriterDelegate<Response>>);

        Command command_;
        Response response_;

      public:
        Task() = default;
        Task(const Task &) = default;
        Task(Task &&) = default;
        Task &operator=(const Task &) = default;
        Task &operator=(Task &&) = default;

        template <typename... Ts>
        Task(Ts &&...ts) : command_{etl::forward<Ts>(ts)...} {}

        inline constexpr decltype(auto) delegate_reader() {
            return command_.delegate_reader();
        }

        inline constexpr decltype(auto) delegate_reader() const {
            return command_.delegate_reader();
        }

        inline constexpr decltype(auto) delegate_writer() {
            return response_.delegate_writer();
        }

        inline constexpr decltype(auto) delegate_writer() const {
            return response_.delegate_writer();
        }

        inline decltype(auto) poll() const {
            return response_.poll();
        }
    };
} // namespace media::uhf::modem
