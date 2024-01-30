#pragma once

#include <nb/serde.h>
#include <stdint.h>

namespace net::local {
    struct LocalNodeConfig {
        friend class AsyncLocalNodeConfigDeserializer;
        friend class AsyncLocalNodeConfigSerializer;

        bool enable_auto_neighbor_discovery : 1 = false;
        bool enable_dynamic_cost_update : 1 = false;
        bool enable_frame_delay : 1 = true;

      private:
        static inline LocalNodeConfig from_byte(uint8_t byte) {
            return LocalNodeConfig{
                .enable_auto_neighbor_discovery = (byte & 0b00000001) != 0,
                .enable_dynamic_cost_update = (byte & 0b00000010) != 0,
                .enable_frame_delay = (byte & 0b00000100) != 0,
            };
        }

        inline uint8_t to_byte() const {
            return (enable_auto_neighbor_discovery ? 1 : 0) |
                (enable_dynamic_cost_update ? 1 : 0) << 1 | (enable_frame_delay ? 1 : 0) << 2;
        }
    };

    class AsyncLocalNodeConfigDeserializer {
        nb::de::Bin<uint8_t> config_;

      public:
        inline LocalNodeConfig result() const {
            return LocalNodeConfig::from_byte(config_.result());
        }

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return config_.deserialize(r);
        }
    };

    class AsyncLocalNodeConfigSerializer {
        nb::ser::Bin<uint8_t> config_;

      public:
        inline AsyncLocalNodeConfigSerializer(const LocalNodeConfig &config)
            : config_(config.to_byte()) {}

        constexpr inline uint8_t serialized_length() const {
            return config_.serialized_length();
        }

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return config_.serialize(w);
        }
    };
} // namespace net::local
