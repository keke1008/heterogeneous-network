#pragma once

#include "./frame.h"
#include "./procedures/address/resolve_address.h"
#include "./procedures/debug/blink.h"
#include "./procedures/dummy/error.h"
#include "./procedures/ethernet/set_ethernet_ip_address.h"
#include "./procedures/ethernet/set_ethernet_subnet_mask.h"
#include "./procedures/local/get_config.h"
#include "./procedures/local/set_cluster_id.h"
#include "./procedures/local/set_config.h"
#include "./procedures/local/set_cost.h"
#include "./procedures/media/get_media_list.h"
#include "./procedures/neighbor/send_hello.h"
#include "./procedures/serial/set_address.h"
#include "./procedures/wifi/close_server.h"
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
            wifi::close_server::Executor,
            serial::set_address::Executor,
            ethernet::set_ethernet_ip_address::Executor,
            ethernet::set_ethernet_subnet_mask::Executor,
            local::set_cost::Executor,
            local::set_cluster_id::Executor,
            local::get_config::Executor,
            local::set_config::Executor,
            neighbor::send_hello::Executor,
            address::resolve_address::Executor>;
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
            case static_cast<uint16_t>(Procedure::CloseServer):
                return wifi::close_server::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SetAddress):
                return serial::set_address::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SetEthernetIpAddress):
                return ethernet::set_ethernet_ip_address::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SetCost):
                return local::set_cost::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SetEthernetSubnetMask):
                return ethernet::set_ethernet_subnet_mask::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SetClusterId):
                return local::set_cluster_id::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::GetConfig):
                return local::get_config::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SetConfig):
                return local::set_config::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::SendHello):
                return neighbor::send_hello::Executor{etl::move(ctx)};
            case static_cast<uint16_t>(Procedure::ResolveAddress):
                return address::resolve_address::Executor{etl::move(ctx)};
            default:
                return dummy::error::Executor{etl::move(ctx), Result::NotImplemented};
            }
        }

      public:
        explicit ProcedureExecutor(RequestContext &&ctx) : executor_{dispatch(etl::move(ctx))} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            link::LinkService &ls,
            net::notification::NotificationService &nts,
            net::local::LocalNodeService &lns,
            net::neighbor::NeighborService &ns,
            util::Time &time,
            util::Rand &rand
        ) {
            return etl::visit(
                util::Visitor{
                    [&](dummy::error::Executor &executor) {
                        return executor.execute(fs, lns, time, rand);
                    },
                    [&](debug::blink::Executor &executor) {
                        return executor.execute(fs, lns, time, rand);
                    },
                    [&](media::get_media_list::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                    [&](wifi::connect_to_access_point::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                    [&](wifi::start_server::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                    [&](wifi::close_server::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                    [&](serial::set_address::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                    [&](ethernet::set_ethernet_ip_address::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                    [&](ethernet::set_ethernet_subnet_mask::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                    [&](local::set_cost::Executor &executor) {
                        return executor.execute(fs, nts, lns, time, rand);
                    },
                    [&](local::set_cluster_id::Executor &executor) {
                        return executor.execute(fs, nts, lns, time, rand);
                    },
                    [&](local::get_config::Executor &executor) {
                        return executor.execute(fs, lns, time, rand);
                    },
                    [&](local::set_config::Executor &executor) {
                        return executor.execute(fs, lns, time, rand);
                    },
                    [&](neighbor::send_hello::Executor &executor) {
                        return executor.execute(fs, ls, lns, ns, time, rand);
                    },
                    [&](address::resolve_address::Executor &executor) {
                        return executor.execute(fs, ms, lns, time, rand);
                    },
                },
                executor_
            );
        }
    };
} // namespace net::rpc
