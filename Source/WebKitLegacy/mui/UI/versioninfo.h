#ifndef __VERSIONINFO_H__
#define __VERSIONINFO_H__

/* OWB_BUILD_DATE, OWB_GIT_COMMIT: written at every build (Source/cmake/OWBBuildInfo.cmake). */
#include "owb_buildinfo.h"

#define VERSION  "2.1"
/* Version shown to the user ($VER, screen title, about page). VERSION itself stays plain,
   since it also goes into the User-Agent string websites see. */
#if defined(__AMIGAOS3__)
/* Release number of the 68k port: r1 was the first Aminet release (it reported itself as
   "2.1-m68k (22.10.2026)"). Set OWB_RELEASE_FINAL to 1 only when building a release;
   other builds show "-dev" and the git commit ("+" = uncommitted changes). */
#define OWB_RELEASE "r2"
#define OWB_RELEASE_FINAL 0
#if OWB_RELEASE_FINAL
#define DISPLAY_VERSION VERSION "-m68k " OWB_RELEASE
#else
#define DISPLAY_VERSION VERSION "-m68k " OWB_RELEASE "-dev " OWB_GIT_COMMIT
#endif
#else
#define DISPLAY_VERSION VERSION
#endif
#define WEBKITVER "605.1.15"
#define REVISION "WebKit " WEBKITVER " (webkitgtk-2.24.4)"

#endif /* __VERSIONINFO_H__ */
