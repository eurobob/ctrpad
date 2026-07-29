# 2026-07-29 Build and Retail-Media Baselines

## Scope

This record establishes the unmodified 32-bit Linux build boundary, the
unmodified Apple ARM64 failure boundary, and the identity of the retail media
currently present in `ref/CTR/`. It does **not** claim an NTSC-U golden run.

## Reproducible Linux i686 build

The checked-in runner is:

```sh
tools/build-linux-i686-baseline.sh
```

It builds `tools/docker/linux-i686.Dockerfile` as
`ctrpad-linux-i686:ubuntu-24.04`. The container uses the pinned Ubuntu image
digest
`sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90`
and an amd64 userspace on the ARM64 development host. The target and linker
flags are both `-m32`.

The first configure failed in SDL because upstream's dependency list omitted
the 32-bit Xcursor development package:

```text
Couldn't find dependency package for XCURSOR
```

The pinned container now includes the required i386 X11/Xcursor/Xext/Xfixes/
Xi/Xrandr, ALSA, OpenGL, udev, and dbus development packages. Its configure
reports:

```text
Platform: Linux-6.4.16-linuxkit
64-bit:   FALSE
Compiler: /usr/bin/cc
Video drivers: dummy x11(dynamic)
X11 libraries: xcursor xdbe xfixes xinput2 xrandr xshape xsync
Render drivers: ogl
Audio drivers: alsa(dynamic) disk dummy
```

The optional JACK, PipeWire, PulseAudio, Wayland, libusb, FriBidi, libthai,
KMSDRM, and liburing backends are absent. None is required by CTR Native's
X11/OpenGL baseline.

The clean build at downstream commit `a76ac25a493d` completed with:

```text
Test #1: ctr_native_version ... Passed
100% tests passed, 0 tests failed out of 1
ELF 32-bit LSB pie executable, Intel 80386
CTR Native 0.1.0-beta.7.1 (a76ac25a493d)
```

The ELF GNU build ID was
`1c1f862bb9bb4112d78a97894bceb0dfd93d37b3`. The exact package manifest is
written to ignored build output
`build-linux-i686-baseline/toolchain-packages.txt`; its primary versions are
GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, and glibc i386 development package
2.39-0ubuntu8.8.

## Unmodified Apple ARM64 configure

On the ARM64 macOS host:

```sh
cmake -S . -B build-macos-arm64-baseline -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

fails before SDL configuration, as intended:

```text
CMake Error at CMakeLists.txt:8 (message):
  CTR Native currently requires a 32-bit target.
```

No source workaround or low-address allocation was used. This is the preserved
M2–M6 starting boundary.

## Retail-media identity

The local CloneCD set has these SHA-256 hashes:

```text
19f4ed5097951e72d2f302ea0a803bb2690e20569a7e50ca812213c15b6c339f  CTR.ccd
84aeb6f990954abb0eed4580fe2e4d2b17ce8a6cb266385b56b96440b523f41a  CTR.img
87d84e9ea413c3d0bcef657b1cf4524a09310a43b25fd75ed615feb124167f7a  CTR.sub
```

`CTR.img` has the expected raw MODE2/2352 container geometry, and the native
disc reader successfully finds its ISO 9660 root and required files. It is not
the NTSC-U disc assumed by `BUILD=926`:

```text
root executable: SCES_021.05
SYSTEM.CNF: BOOT = cdrom:\SCES_021.05;1
```

`SCES_021.05` is the PAL Europe executable. The expected NTSC-U identity is
`SCUS_944.26`.

Before an identity gate existed, the 32-bit build reached `StateZero`, used
NTSC-U BIGFILE index `0x1fd`, and interpreted unrelated PAL data as a
`VramHeader`. The abort trace was:

```text
NativeRenderer_CopyVRAM      platform/native_renderer.c:2018
LOAD_VramFileCallback        game/LOAD/LOAD_File.c:208
LOAD_VramFile                game/LOAD/LOAD_File.c:257
StateZero                    game/MAIN/MainMain.c:643
CTR_Main                     game/MAIN/MainMain.c:104
```

Direct inspection confirms PAL entry `0x1fd` begins with unrelated data and an
impossible rectangle. This is an input/build-region mismatch, not evidence of
a valid NTSC-U loader failure.

The runtime now extracts the boot disc ID from `SYSTEM.CNF`, rejects anything
other than `SCUS_944.26` before game initialization, and bounds-checks every
VRAM copy as defense in depth. A true M1 golden run remains pending a
user-supplied NTSC-U raw MODE2/2352 image.
