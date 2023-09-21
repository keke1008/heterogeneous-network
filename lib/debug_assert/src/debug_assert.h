#pragma once

#if __has_include(<Arduino.h>)

#define DEBUG_ASSERT(condition, ...)

#else

#include <cassert>

#define DEBUG_ASSERT(condition, ...) assert(condition && " " __VA_ARGS__)

#endif
