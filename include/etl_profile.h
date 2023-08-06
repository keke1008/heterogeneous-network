#pragma once
#define ETL_NO_STL

#if not __has_include(<Arduino.h>)

#define ETL_FORCE_STD_INITIALIZER_LIST

#endif
