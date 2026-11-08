/* Minimal mman.h for AmigaOS3 m68k: sqlite uses mmap() only as an optional
 * read accelerator. These stubs report failure so sqlite falls back to plain
 * pread() I/O, which is correct, just not cache-optimised. */
#ifndef _COMPAT_SYS_MMAN_H
#define _COMPAT_SYS_MMAN_H
#include <sys/types.h>
#define PROT_NONE  0
#define PROT_READ  1
#define PROT_WRITE 2
#define PROT_EXEC  4
#define MAP_SHARED     1
#define MAP_PRIVATE    2
#define MAP_FIXED      0x10
#define MAP_ANONYMOUS  0x20
#define MAP_FAILED     ((void *)-1)
static inline void *mmap(void *addr, size_t len, int prot, int flags, int fd, long off) {
    (void)addr; (void)len; (void)prot; (void)flags; (void)fd; (void)off;
    return MAP_FAILED;
}
static inline int munmap(void *addr, size_t len) { (void)addr; (void)len; return -1; }
#endif
