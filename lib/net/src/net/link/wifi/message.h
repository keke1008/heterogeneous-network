#pragma once

#include "./link.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/string_view.h>
#include <stdint.h>
#include <util/span.h>

namespace net::link::wifi::message {
    struct ReceiveData {
        LinkId link_id;
        uint32_t length;

        static etl::optional<ReceiveData> try_parse(etl::span<const uint8_t> line);
    };

    struct ConnectionClosed {
        LinkId link_id;

        static etl::optional<ConnectionClosed> try_parse(etl::span<const uint8_t> line);
    };

    struct ConnectionEstablished {
        LinkId link_id;

        static etl::optional<ConnectionEstablished> try_parse(etl::span<const uint8_t> line);
    };

    enum class ResponseType : uint8_t {
        NEVER,
        OK,
        ERROR,
        FAIL,
        SEND_OK,
        SEND_FAIL,
    };

    inline constexpr etl::string_view as_string_view(ResponseType type) {
        switch (type) {
        case ResponseType::NEVER:
            return "ERROR\r\n";
        case ResponseType::OK:
            return "OK\r\n";
        case ResponseType::ERROR:
            return "ERROR\r\n";
        case ResponseType::FAIL:
            return "FAIL\r\n";
        case ResponseType::SEND_OK:
            return "SEND OK\r\n";
        case ResponseType::SEND_FAIL:
            return "SEND FAIL\r\n";
        default:
            DEBUG_ASSERT(false, "Unreachable");
        };
    }

    template <ResponseType T>
    constexpr uint8_t response_type_length = as_string_view(T).size();

    template <ResponseType T>
    class Response {
      public:
        static constexpr ResponseType type = T;

        static bool try_parse(etl::span<const uint8_t> line) {
            constexpr auto str = as_string_view(T);
            return util::span::equal(line, str);
        }
    };
} // namespace net::link::wifi::message
