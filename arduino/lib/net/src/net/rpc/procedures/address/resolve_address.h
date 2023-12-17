#pragma once

#include "../../request.h"
#include "nb/poll.h"
#include "net/node/node_id.h"
#include <nb/serde.h>

namespace net::rpc::address::resolve_address {
    struct Param {
        node::NodeId target_id;
    };

    class AsyncParameterDeserializer {
        node::AsyncNodeIdDeserializer target_id_;

      public:
        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return target_id_.deserialize(r);
        }

        inline Param result() {
            return Param{
                .target_id = target_id_.result(),
            };
        }
    };

    using AsyncResultSerializer = nb::ser::Vec<link::AsyncAddressSerializer, link::MAX_MEDIA_PORT>;

    template <nb::AsyncReadableWritable RW>
    class Executor {
        RequestContext<RW> ctx_;
        AsyncParameterDeserializer param_;
        etl::optional<AsyncResultSerializer> result_;

      public:
        explicit Executor(RequestContext<RW> &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            link::LinkService<RW> &ls,
            const node::LocalNodeService &lns,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_ready_to_send_response()) {
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            if (!result_) {
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(param_));
                const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
                if (info.source.node_id != param_.result().target_id) {
                    return nb::ready(); // ignore frame
                }

                etl::vector<link::Address, link::MAX_MEDIA_PORT> addresses;
                ls.get_media_addresses(addresses);
                result_.emplace(etl::move(addresses));
                ctx_.set_response_property(Result::Success, result_->serialized_length());
            }

            auto writer = POLL_UNWRAP_OR_RETURN(ctx_.poll_response_writer(fs, lns, rand));
            POLL_UNWRAP_OR_RETURN(writer.get().serialize(*result_));
            return ctx_.poll_send_response(fs, lns, time, rand);
        }
    };
} // namespace net::rpc::address::resolve_address
