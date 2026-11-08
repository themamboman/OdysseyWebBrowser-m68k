# Building OWB for AmigaOS 3.x (m68k)

> **Work in progress.** This describes the environment the first release was built in. A
> step-by-step guide from a clean machine, with scripts, is still to come. Expect to adapt
> paths and to rebuild some libraries yourself.

OWB is cross-compiled on Linux. The build needs three things: a patched m68k-amigaos-gcc
toolchain with clib2, a set of third-party libraries built for m68k and installed into that
toolchain, and this repository.

## 1. Get the source

```bash
git clone --recurse-submodules -b amigaos3-m68k https://github.com/themamboman/OdysseyWebBrowser-m68k.git
```

The `aos_multithreading_library` submodule provides WebKit's threading on AmigaOS 3.x. If a
checkout of it exists beside this repository, the build uses that one instead.

## 2. Toolchain

- **Compiler:** [AmigaPorts m68k-amigaos-gcc](https://github.com/AmigaPorts/m68k-amigaos-gcc),
  commit `124eb52` (GCC 16.2.0b), installed to a prefix such as `/opt/amiga`.
- **C library:** clib2 (`projects/clib2`, commit `842c0c6`) with
  [`patches/clib2-842c0c6.patch`](patches/clib2-842c0c6.patch), a fix to BSS clearing in the
  base-relative startup code.
- **Flags:** `-mcpu=68040 -fbaserel -fno-pic -mcrt=clib2`. The resulting code also runs on a
  68020 with a 68881/68882 FPU.

Three fixes to the installed toolchain were needed:

1. **libgcc and libstdc++ rebuilt without `-fbaserel`.** The prebuilt `libb/libm020` variants
   overflow 16-bit base-relative relocations in a program this size. In the GCC build tree,
   under `gcc/m68k-amigaos/libb/libm020`, replace `-fbaserel` with `-fno-pic` in the generated
   Makefiles of `libgcc` and `libstdc++-v3`, run `make clean && make` in each, and copy the
   resulting `libgcc.a`, `libstdc++.a` into
   `<prefix>/lib/gcc/m68k-amigaos/16.2.0b/libb/libm020/`. Delete any 0-byte `.c`/`.cc` files in
   the build tree first: they override the real sources.
2. **AmiSSL autoinit object.** `libamisslauto.a` registers its init/exit functions through a.out
   set symbols, which makes the linker drop every C++ static constructor. Rename the symbols
   in a copy of `autoinit_assl.o` (same-length edit: `___CTOR_LIST__` to `___AMCTR_LIST_`,
   `___DTOR_LIST__` to `___AMDTR_LIST_`) and reinstall it. OWB's `main()` calls
   `__init_amissl()` itself.
3. **Thread-safe malloc.** Link clib2's thread-safe allocator members (`stdlib_malloc_ts.o`,
   `stdlib_free_ts.o`, `stdlib_calloc_ts.o`, `stdlib_realloc_ts.o`, `stdlib_alloca_ts.o`) plus
   a `strtold` stub (`strtod` cast to `long double`) as a whole archive, before the C library:
   `CMAKE_EXE_LINKER_FLAGS="-mcrt=clib2 -Wl,--whole-archive <path>/libclib2ts.a -Wl,--no-whole-archive -lamiga -lm -lgcc"`.

## 3. Libraries

All are static, built with the toolchain flags above and installed into `<prefix>`
(`<prefix>/m68k-amigaos/{include,lib}` unless noted).

| Library | Version | Changes |
|---|---|---|
| AmiSSL SDK | 5.27 (API `AMISSL_V362`) | see toolchain fix 2 |
| curl | 8.12.1 | none, built against AmiSSL |
| cairo | 1.18.0 | [`patches/cairo-1.18.0.patch`](patches/cairo-1.18.0.patch) |
| pixman | 0.44.2 | [`patches/pixman-0.44.2.patch`](patches/pixman-0.44.2.patch) |
| FreeType | 2.12.1 | none |
| HarfBuzz | 8.4.0 | none |
| fontconfig | minimal replacement | [`amiga/support/fontconfig-minimal`](../../amiga/support/fontconfig-minimal) |
| ICU | 55.2 | data shipped as files in `Dist/Resource/icudt55b/` |
| libxml2 | 2.15.1 | none; installed in `<prefix>/{include,lib}` |
| libxslt | 1.1.42 | installed in `<prefix>/{include,lib}` |
| SQLite | 3.52.0 (autoconf) | [`patches/sqlite-3.52.0.patch`](patches/sqlite-3.52.0.patch); installed in `<prefix>/lib` |
| expat | stub | [`amiga/support/expat-stub`](../../amiga/support/expat-stub); [`patches/expat-2.7.4.patch`](patches/expat-2.7.4.patch) is for the real 2.7.4 tree |
| libpng | 1.6.58 | none |
| libjpeg | 9e | none |
| zlib | 1.3.2 | none |
| bzip2 | 1.0.8 | none |

SQLite was built with:

```bash
m68k-amigaos-gcc -mcpu=68040 -fbaserel -fno-pic -mcrt=clib2 -O1 -I. \
  -include clib2_sqlite.h -DSQLITE_ENABLE_MATH_FUNCTIONS -DSQLITE_ENABLE_PERCENTILE \
  -DSQLITE_HAVE_ZLIB=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=1 \
  -DSQLITE_OMIT_WAL=1 -DSQLITE_MAX_MMAP_SIZE=0 -c sqlite3.c
```

### Small support libraries ([`amiga/support`](../../amiga/support))

- `amigaposix/`: POSIX functions clib2 lacks (`libamigaposix.a`). **Build with
  `-O0 -fno-builtin`**: at higher levels GCC turns parts of it into calls back to itself.
  `clib2_sqlite.h` is the header SQLite is compiled with.
- `asyncio/`: `libasyncio.a` shim.
- `expat-stub/`: `libexpat.a`.
- `fontconfig-minimal/`: `libfontconfig.a`.

## 4. Configure and build

```bash
mkdir build-amiga && cd build-amiga
cmake .. -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE=../Source/cmake/m68k-amigaos.cmake \
  -DWEBKIT_AMIGA_TOOLCHAIN_PREFIX=/opt/amiga \
  -DPORT=MUI -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_EXE_LINKER_FLAGS="<see toolchain fix 3>"
make -j4 OWB
m68k-amigaos-strip -o ../Dist/OWB bin/OWB
```

`RelWithDebInfo` is `-O1` here (see `Source/cmake/m68k-amigaos.cmake`); the first release uses
it. An `-O2` build also works but has not been fully tested yet.

## 5. Run

Everything OWB needs at run time is in `Dist/`. Start it with a 64,000,000-byte stack: the
`OWB.info` icon sets this. ICU data must be the `Resource/icudt55b/` tree of files, not a
`.dat` file.

For release archives, leave out `Dist/Conf` apart from `Conf/font` and `Conf/blocked.prefs`.

## Porting notes

Points that cost the most time, in case you port similar code:

- m68k GCC aligns pointers to 2 bytes. Code that keeps flags in the low bits of a static
  address breaks; JavaScriptCore's `LazyProperty` needed `alignas(4)`.
- clib2's soft-float double comparisons get NaN wrong, and several maths functions (`log10`,
  `fmod`, `pow`, `atan2`, ...) needed replacements; see `Tools/OdysseyWebBrowser/main.cpp`.
- AmigaDOS `Seek()` fails past the end of a file, unlike POSIX `lseek()`; SQLite needed a patch.
- WebKit threads must be DOS processes, with their own bsdsocket.library and AmiSSL bases.
- Never let `fastMalloc()` return NULL to WebKit: callers write through it into low memory.
