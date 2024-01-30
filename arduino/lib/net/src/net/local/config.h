#pragma once

namespace net::local {
    struct LocalNodeConfig {
        bool enable_auto_neighbor_discovery : 1 = false;
        bool enable_dynamic_cost_update : 1 = false;
    };
} // namespace net::local
