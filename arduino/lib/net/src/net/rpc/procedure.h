#pragma once

#include "./frame.h"
#include "./procedures/debug/blink.h"
#include "./procedures/dummy/error.h"
#include "./procedures/media/get_media_list.h"
#include "./procedures/neighbor/send_goodbye.h"
#include "./procedures/neighbor/send_hello.h"
#include "./procedures/wifi/connect_to_access_point.h"
#include "./procedures/wifi/start_server.h"
#include "./request.h"
#include <util/visitor.h>

namespace net::rpc {
    template <nb::AsyncReadableWritable RW>
    class ProcedureExecutor {
        using Executor = etl::variant<
            dummy::error::Executor<RW>,
            debug::blink::Executor<RW>,
            media::get_media_list::Executor<RW>,
            wifi::connect_to_access_point::Executor<RW>,
            wifi::start_server::Executor<RW>,
            neighbor::send_hello::Executor<RW>,
            neighbor::send_goodbye::Executor<RW>>;
        Executor executor_;

        static Executor dispatch(RequestContext<RW> &&ctx) {
            auto procedure = ctx.request().procedure();
            switch (procedure) {
            case static_cast<uint16_t>(Procedure::Blink):
                return debug::blink::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::GetMediaList):
                return media::get_media_list::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::ConnectToAccessPoint):
                return wifi::connect_to_access_point::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::StartServer):
                return wifi::start_server::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SendHello):
                return neighbor::send_hello::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SendGoodbye):
                return neighbor::send_goodbye::Executor{etl::move(ctx)};
            default:
                return dummy::error::Executor{etl::move(ctx), Result::NotImplemented};
            }
        }

      public:
        explicit ProcedureExecutor(RequestContext<RW> &&ctx)
            : executor_{dispatch(etl::move(ctx))} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            node::LocalNodeService &lns,
            link::LinkService<RW> &ls,
            routing::RoutingService<RW> &rs,
            util::Time &time,
            util::Rand &rand
        ) {
            return etl::visit(
                util::Visitor{
                    [&](dummy::error::Executor<RW> &executor) {
                        return executor.execute(fs, lns, time, rand);
                    },
                    [&](debug::blink::Executor<RW> &executor) {
                        return executor.execute(fs, lns, time, rand);
                    },
                    [&](media::get_media_list::Executor<RW> &executor) {
                        return executor.execute(fs, ls, lns, time, rand);
                    },
                    [&](wifi::connect_to_access_point::Executor<RW> &executor) {
                        return executor.execute(fs, ls, lns, time, rand);
                    },
                    [&](wifi::start_server::Executor<RW> &executor) {
                        return executor.execute(fs, ls, lns, time, rand);
                    },
                    [&](neighbor::send_hello::Executor<RW> &executor) {
                        return executor.execute(fs, ls, lns, rs, time, rand);
                    },
                    [&](neighbor::send_goodbye::Executor<RW> &executor) {
                        return executor.execute(fs, ls, lns, rs, time, rand);
                    },
                },
                executor_
            );
        }
    };
} // namespace net::rpc
