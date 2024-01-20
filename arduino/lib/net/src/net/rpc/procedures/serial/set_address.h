#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::serial::set_address {
    struct Param {
        link::MediaPortNumber port_number;
        link::Address address;
    };

    class AsyncParameterDeserializer {
        link::AsyncMediaPortNumberDeserializer port_number_;
        link::AsyncAddressDeserializer address_;

      public:
        inline Param result() const {
            return Param{
                .port_number = port_number_.result(),
                .address = address_.result(),
            };
        };

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(port_number_.deserialize(reader));
            return address_.deserialize(reader);
        }
    };

    class Executor {
        RequestContext ctx_;
        AsyncParameterDeserializer param_;

      public:
        explicit Executor(RequestContext &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            const net::local::LocalNodeService &lns,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            auto result = POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(param_));
            if (result != nb::DeserializeResult::Ok) {
                ctx_.set_response_property(Result::BadArgument, 0);
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            const auto &param = param_.result();
            auto opt_ref_port = ms.get_media_port(param.port_number);
            if (!opt_ref_port.has_value()) {
                ctx_.set_response_property(Result::InvalidOperation, 0);
            }

            auto &port = opt_ref_port->get();
            link::MediaPortOperationResult res =
                port.serial_try_initialize_local_address(param.address);
            switch (res) {
            case link::MediaPortOperationResult::Success:
                ctx_.set_response_property(Result::Success, 0);
                break;
            case link::MediaPortOperationResult::Failure:
                ctx_.set_response_property(Result::Failed, 0);
                break;
            case link::MediaPortOperationResult::UnsupportedOperation:
                ctx_.set_response_property(Result::InvalidOperation, 0);
                break;
            }

            return ctx_.poll_send_response(fs, lns, time, rand);
        }
    };
}; // namespace net::rpc::serial::set_address
