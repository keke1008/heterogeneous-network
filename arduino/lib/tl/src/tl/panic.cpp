#include "./panic.h"

namespace tl {
    [[noreturn]] void halt() {
        while (true) {}
    }
} // namespace tl
