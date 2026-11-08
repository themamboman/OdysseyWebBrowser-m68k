#ifndef __VERSIONINFO_H__
#define __VERSIONINFO_H__

#define VERSION  "2.1"
/* Version shown to the user ($VER, screen title). VERSION itself stays plain,
   since it also goes into the User-Agent string websites see. */
#if defined(__AMIGAOS3__)
#define DISPLAY_VERSION VERSION "-m68k"
#else
#define DISPLAY_VERSION VERSION
#endif
#define WEBKITVER "605.1.15"
#define REVISION "WebKit " WEBKITVER " (webkitgtk-2.24.4)"

#endif /* __VERSIONINFO_H__ */
