// This AROS-compat shim used to wrap <aros/debug.h>, which doesn't exist on AmigaOS3. The real
// m68k-amigaos NDK (reached via #include_next, since this directory's own -I entry comes first)
// already provides a proper clib/debug_protos.h with kprintf() and the rest of debug.lib's API,
// which is a strict superset of what callers here actually use. No include guard of our own:
// the real header has its own (under the same macro name), which is what actually protects
// against multiple inclusion once we hand off to it.
#include_next <clib/debug_protos.h>
