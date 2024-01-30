#pragma once

namespace net::local {
    struct LocalNodeConfig {
        bool enable_auto_neighbor_discovery : 1 = false;
    };
} // namespace net::local
