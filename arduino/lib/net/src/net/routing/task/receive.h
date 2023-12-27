#pragma once

#include "../frame.h"
#include <net/neighbor.h>

namespace net::routing::task {
    class ReceiveFrameTask {
        struct Deserialize {
            neighbor::NeighborFrame frame;
            AsyncRoutingFrameHeaderDeserializer deserializer{};

            explicit Deserialize(neighbor::NeighborFrame &&frame) : frame{etl::move(frame)} {}
        };

        struct Result {
            etl::optional<RoutingFrame> frame;

            Result(etl::optional<RoutingFrame> &&frame) : frame{etl::move(frame)} {}

            Result(const Result &) = delete;
        };

        etl::variant<Deserialize, Result> state_;

      public:
        explicit ReceiveFrameTask(neighbor::NeighborFrame &&frame)
            : state_{Deserialize{etl::move(frame)}} {}

        template <nb::AsyncReadableWritable RW, uint8_t N>
        inline nb::Poll<void> execute(
            neighbor::NeighborService<RW> &ns,
            frame::FrameIdCache<N> &frame_id_cache,
            const local::LocalNodeInfo &local,
            util::Time &time
        ) {
            if (etl::holds_alternative<Deserialize>(state_)) {
                auto &&[frame, deserializer] = etl::get<Deserialize>(state_);
                auto result = POLL_UNWRAP_OR_RETURN(frame.reader.deserialize(deserializer));
                if (result != nb::DeserializeResult::Ok) {
                    state_.emplace<Result>(etl::nullopt);
                    return nb::ready();
                }

                RoutingFrame &&deserialized = deserializer.as_frame(frame.reader.subreader());
                if (frame_id_cache.insert_and_check_contains(deserialized.frame_id)) {
                    state_.emplace<Result>(etl::nullopt);
                    return nb::ready();
                }

                state_.emplace<Result>(RoutingFrame{etl::move(deserialized)});
            }

            return nb::ready();
        }

        inline etl::optional<RoutingFrame> &result() {
            FASSERT(etl::holds_alternative<Result>(state_));
            return etl::get<Result>(state_).frame;
        }
    };
} // namespace net::routing::task
