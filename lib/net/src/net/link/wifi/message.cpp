#include "./message.h"

#include <serde/dec.h>
#include <util/span.h>
#include <util/structured_bindings.h>

namespace net::link::wifi::message {
    etl::optional<ReceiveData> ReceiveData::try_parse(etl::span<const uint8_t> line) {
        if (!util::span::starts_with(line, "+IPD")) {
            return etl::nullopt;
        }

        auto split_result = util::span::splitn_by<3>(util::span::drop_crlf(line), ',');
        if (!split_result.has_value()) {
            return etl::nullopt;
        }

        const auto [_header, link_id, length] = split_result.value();
        return ReceiveData{
            .link_id = static_cast<LinkId>(link_id[0]),
            .length = serde::dec::deserialize<uint32_t>(length),
        };
    }

    etl::optional<ConnectionClosed> ConnectionClosed::try_parse(etl::span<const uint8_t> line) {
        if (!util::span::starts_with(line, "CLOSED\r\n")) {
            return etl::nullopt;
        }

        uint8_t link_id = serde::dec::deserialize<uint8_t>(line.first<1>());
        return ConnectionClosed{
            .link_id = static_cast<LinkId>(link_id),
        };
    }

    etl::optional<ConnectionEstablished>
    ConnectionEstablished::try_parse(etl::span<const uint8_t> line) {
        if (!util::span::starts_with(line, "CONNECT\r\n")) {
            return etl::nullopt;
        }

        uint8_t link_id = serde::dec::deserialize<uint8_t>(line.first<1>());
        return ConnectionEstablished{
            .link_id = static_cast<LinkId>(link_id),
        };
    }
} // namespace net::link::wifi::message
