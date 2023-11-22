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
    class ProcedureExecutor {
        using Executor = etl::variant<
            dummy::error::Executor,
            debug::blink::Executor,
            media::get_media_list::Executor,
            wifi::connect_to_access_point::Executor,
            wifi::start_server::Executor,
            neighbor::send_hello::Executor,
            neighbor::send_goodbye::Executor>;
        Executor executor_;

        static Executor dispatch(RequestContext &&ctx) {
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
        explicit ProcedureExecutor(RequestContext &&ctx) : executor_{dispatch(etl::move(ctx))} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            link::LinkService &ls,
            routing::RoutingService &rs,
            util::Time &time,
            util::Rand &rand
        ) {
            return etl::visit(
                util::Visitor{
                    [&](dummy::error::Executor &executor) {
                        return executor.execute(fs, rs, time, rand);
                    },
                    [&](debug::blink::Executor &executor) {
                        return executor.execute(fs, rs, time, rand);
                    },
                    [&](media::get_media_list::Executor &executor) {
                        return executor.execute(fs, rs, ls, time, rand);
                    },
                    [&](wifi::connect_to_access_point::Executor &executor) {
                        return executor.execute(fs, rs, ls, time, rand);
                    },
                    [&](wifi::start_server::Executor &executor) {
                        return executor.execute(fs, rs, ls, time, rand);
                    },
                    [&](neighbor::send_hello::Executor &executor) {
                        return executor.execute(fs, rs, ls, time, rand);
                    },
                    [&](neighbor::send_goodbye::Executor &executor) {
                        return executor.execute(fs, rs, ls, time, rand);
                    },
                },
                executor_
            );
        }
    };
} // namespace net::rpc
