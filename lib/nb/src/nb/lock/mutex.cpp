#include "mutex.h"

namespace nb::lock {
    Mutex::Mutex(){};

    bool Mutex::lock() {
        bool lockable = !locked_;
        locked_ = true;
        return lockable;
    }

    void Mutex::unlock() {
        locked_ = false;
    }

    bool Mutex::is_locked() const {
        return locked_;
    }
} // namespace nb::lock
