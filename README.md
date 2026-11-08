# OdysseyWebBrowser for classic AmigaOS 3.x (m68k)

A port of [Odyssey Web Browser](https://github.com/deadwood2/OdysseyWebBrowser) (OWB), the
WebKit-based browser for MorphOS, AROS and AmigaOS 4, to classic 68k AmigaOS 3.x. It is based
on WebKitGTK 2.24.4 (February 2019) and renders modern sites with HTML5, CSS and JavaScript,
with HTTPS through AmiSSL.

**Status: early test release.** It works, but it is slow and needs a lot of memory. Binaries
are released on [Aminet](https://aminet.net/) as `OWB-m68k.lha`.

This branch, `amigaos3-m68k`, is based on upstream's `odyssey` branch.

## Requirements

- AmigaOS 3.x, 68020 or better with an FPU
- RTG graphics (Picasso96 or CyberGraphX)
- MUI, AmiSSL 5.27 or newer, a TCP/IP stack
- 256 MB or more of fast RAM

Tested so far only in WinUAE (AmiKit XE): 68020 + 68881 with JIT, UAEgfx RTG, 384 MB.

## Building

See [docs/amiga/BUILDING.md](docs/amiga/BUILDING.md). It cross-compiles on Linux with the
AmigaPorts m68k-amigaos-gcc (GCC 16) and clib2. The guide describes the environment the first
release was built in; a from-scratch guide is in progress.

```bash
git clone --recurse-submodules -b amigaos3-m68k https://github.com/themamboman/OdysseyWebBrowser-m68k.git
```

## Layout of the port

| Path | Contents |
|---|---|
| `Source/cmake/m68k-amigaos.cmake` | CMake toolchain file |
| `Source/WTF/wtf/amiga/` | Threading, memory allocator and file system for AmigaOS 3.x |
| `aos_multithreading_library/` | Submodule: [aos_multithreading_library](https://github.com/themamboman/aos_multithreading_library) |
| `amiga/` | RTG layer, header stubs and small support libraries |
| `docs/amiga/` | Build guide and patches to third-party libraries |
| `Tools/OdysseyWebBrowser/main.cpp` | Start-up, plus replacements for broken C library maths |

## Known issues

- Slow: JavaScript-heavy pages can take a minute or more
- No right-click context menu
- `file:///` URLs do not open
- Running out of memory can crash the whole system
- No video or audio; YouTube does not load

## Credits and license

- Odyssey Web Browser: Fabien Coeurjoly, Jacek Piszczek and Krzysztof Smiechowicz
  (deadwood); see the upstream repository
- WebKit: Apple, the WebKitGTK team and contributors
- 68k port: TheMamboMan, with help from Claude (Anthropic)

WebKit is licensed under the LGPL 2.1 ([LICENSE.LGPL](LICENSE.LGPL)) and BSD-style licenses;
OWB's own code is under a BSD-style license (see the readme in `Dist/`). The bundled fonts are
Liberation Sans and Mono under the SIL Open Font License 1.1 (`Dist/Fonts/LICENSE.txt`).
