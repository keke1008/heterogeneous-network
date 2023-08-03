#pragma once

#if __has_attribute(__ATTR_PROGMEM__)

#define PROGMEM __ATTR_PROGMEM__

#else

#define PROGMEM

#endif
