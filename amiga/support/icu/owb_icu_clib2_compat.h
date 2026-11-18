/* Forced include for building ICU 55 against clib2 (AmigaOS 3.x / OWB). */
#ifndef OWB_ICU_CLIB2_COMPAT_H
#define OWB_ICU_CLIB2_COMPAT_H
#include <locale.h>
/* The AmigaOS pthread.h wraps cancellation points as macros, e.g.
   #define system(...) (pthread_testcancel(), system(__VA_ARGS__)); if a clib2
   header declaring them is read after pthread.h, the declaration breaks.
   Include those headers first. */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
/* clib2 has no LC_MESSAGES; ICU only uses it to read the default locale,
   treating it exactly like LC_CTYPE. */
#ifndef LC_MESSAGES
#define LC_MESSAGES LC_CTYPE
#endif
/* clib2 has no tzname[]; ICU's putilimp.h uses a predefined U_TZNAME. */
static const char *const owb_icu_tzname[2] = { "UTC", "UTC" };
#define U_TZNAME owb_icu_tzname
#endif
