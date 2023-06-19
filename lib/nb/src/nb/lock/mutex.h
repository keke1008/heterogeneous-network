#pragma once

namespace nb::lock {
    class Mutex {
        bool locked_ = false;

      public:
        Mutex();
        bool lock();
        void unlock();
    };
} // namespace nb::lock
