#pragma once

namespace net::local {
    struct LocalNodeConfig {
        bool enable_auto_neighbor_discovery : 1 = false;
        bool enable_dynamic_cost_update : 1 = false;
        bool enable_frame_delay : 1 = true;
    };
} // namespace net::local
