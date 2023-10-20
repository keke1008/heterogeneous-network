#pragma once

#if __has_include(<avr/pgmspace.h>)

#include <avr/pgmspace.h>

#else

#define PROGMEM

#endif
