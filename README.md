# CTRPad

<p align="center">
  <img src="docs/design/ctrpad-app-icon-master.png" width="160" alt="CTRPad app icon">
</p>

Crash Team Racing rebuilt as a native app for iPhone, iPad, Apple Silicon Mac,
Windows, and Linux.

CTRPad is a native port built from the
[CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) decompilation project. On
iOS and iPadOS it provides Metal-backed presentation, Files-based disc import,
customizable landscape touch controls, keyboard input, and SDL's native game
controller path. Desktop builds provide resizable windows, native fullscreen,
keyboard controls, and mouse-button gameplay shortcuts.

This repository contains source code, platform integration, documentation, and
retail-free build tooling. It does **not** contain Crash Team Racing, disc
images, extracted retail assets, saves, signing credentials, or other
copyrighted game data. You must supply your own compatible NTSC-U retail disc
image.

CTRPad is an unofficial community project. It is not affiliated with or
endorsed by Sony, PlayStation, Naughty Dog, or Activision.

## Install status

| Target | Status | Recommended path |
|---|---|---|
| iPhone and iPad | User-signed development build | Download or build the unsigned IPA, then sign it with your Apple ID; see [the iOS installation guide](docs/INSTALL-IOS.md) |
| iOS Simulator | Available for development | Build with the Simulator preset and boot exactly one Simulator |
| Apple Silicon Mac | Packaged native application | Use a release that includes the macOS ZIP, or build it locally; move CTRPad to Applications and choose your BIN on first launch |
| Windows | Native x86 builds available | Use MSVC or MinGW |
| Linux | Native i686 build available | Use the Linux build script or CMake preset |
| App Store / TestFlight | Not announced | No official listing or public TestFlight exists |

The iPhone and iPad Simulator builds have been exercised for disc import,
launch, saves, touch gameplay, touch-layout editing, rotation, app updates, and
background/foreground input cleanup. Simulator evidence is not physical-device
proof. Real-device touch ergonomics and a representative Bluetooth/MFi
controller matrix remain acceptance checks.

## Get started

Every build requires your own NTSC-U single-track raw MODE2/2352 Crash Team
Racing BIN. CTRPad and its release artifacts contain no game data.

### Download a build

- **Apple Silicon Mac:** when a [GitHub release](https://github.com/chrissotraidis/ctrpad/releases)
  includes `CTRPad-macOS-arm64-*.zip`, download it, move `CTRPad.app` to
  Applications, and open it. The app prompts for your BIN and remembers its
  external location. See [Install CTRPad on macOS](docs/INSTALL-MACOS.md).
- **iPhone or iPad:** when a release includes an unsigned `.ipa`, download and
  re-sign it with your own Apple ID, or build/sign locally. See
  [Build, sign, and sideload CTRPad](docs/INSTALL-IOS.md).
- **Windows or Linux:** use a matching release archive when provided, or use
  the source-build instructions below.

iOS/iPadOS packages cannot be installed unsigned, and no App Store or public
TestFlight build is currently offered. Ad-hoc macOS developer previews may
require Control-click → **Open** once; a notarized release opens normally.

### Build from source

You need:

- a clone of this repository;
- CMake 3.20 or newer;
- Ninja for the Apple presets;
- Xcode and its command-line tools for Apple builds; and
- your compatible retail BIN.

Clone the project:

```sh
git clone https://github.com/chrissotraidis/ctrpad.git
cd ctrpad
```

### iPhone and iPad Simulator

```sh
cmake --preset ios-simulator-arm64
cmake --build --preset ios-simulator-arm64
```

Boot exactly one iPhone or iPad Simulator, then use the guarded installer:

```sh
./tools/install-ios-simulator.sh \
  --device YOUR_BOOTED_SIMULATOR_UDID \
  --launch
```

The installer signs an isolated staging copy and verifies that the installed
executable matches the product that was just built. See
[`docs/INSTALL-IOS.md`](docs/INSTALL-IOS.md) for physical-device signing,
direct installation, and user-side IPA re-signing.

### Physical iPhone or iPad build

```sh
cmake --preset ios-device-arm64
cmake --build --preset ios-device-arm64
./package-ios.sh
```

The default package is unsigned and contains no provisioning profile or
maintainer identity. It must be signed with the installing user's own Apple
credentials and a compatible profile. `package-ios.sh --build` combines the
device build and packaging steps.

### Apple Silicon Mac

```sh
cmake --preset macos-arm64-app
cmake --build --preset macos-arm64-app
ctest --preset macos-arm64-app
```

Open `build-macos-arm64-app/CTRPad.app`. First launch presents a native file
picker, validates the selected BIN, remembers the external path, and starts
the game. Saves and logs live under Application Support rather than beside or
inside the app. `./package-macos.sh --build` creates a retail-free ZIP.

## First launch on Apple platforms

CTRPad never downloads or bundles game data.

On iPhone or iPad:

1. Launch CTRPad and select **Choose CTR disc image**.
2. Pick your own compatible NTSC-U raw BIN through Files.
3. Wait while CTRPad validates and imports the image.
4. The game starts in the same app process after a valid import.

The imported image is stored privately under
`Documents/CTRPad/assets/ctr-u.bin`. Invalid or cancelled selections do not
replace a previously verified import. **Change Disc** remains available at the
upper-right edge of the game screen.

On macOS, launch `CTRPad.app` and choose the BIN in the native first-launch
panel. The app remembers the file's external location without placing game
data inside the bundle. Use `CTRPad --choose-disc` to select a different image.

For desktop development builds, place the same image at `assets/ctr-u.bin`
beside the source or packaged executable. A cooked 2048-byte ISO is not a
substitute: it omits the XA/STR raw-sector data needed by the game.

## Touch controls

CTRPad provides separate safe-area-aware landscape defaults for iPhone and
iPad. The steering stick stays under one thumb while Gas, Brake, Item, View,
Start, Select, and both Drift/Boost buttons form a reachable action cluster
under the other. Right-hand steering mirrors the gameplay layout.

| Touch control | PlayStation input | Typical use |
|---|---|---|
| Steer | Left analog stick | Continuous steering |
| Stick outer ring | D-pad | Menus and digital direction input |
| **Gas ✕** | Cross | Accelerate and confirm |
| **Brake □** | Square | Brake, reverse, and menu action |
| **Item ○** | Circle | Use item and menu action |
| **View △** | Triangle | Camera, skip, and menu action |
| **L Drift / Boost** | L1 | Hop, drift, and boost |
| **R Drift / Boost** | R1 | Alternate hop and drift side |
| **Start / Pause** | Start | Start, advance, pause, or resume |
| **Select** | Select | Retail Select input |

### Customize the layout

1. Open **Controls** at the upper right.
2. Turn **On-screen controls** off when using a physical controller, or leave
   it on to use touch.
3. Choose **Edit touch layout**.
4. Drag any gameplay control to reposition it.
5. Select a control and use **−** or **+** to resize it from 70% to 150%.
6. Choose **Done** to return to play, or **Reset** to restore the CTRPad
   default for the current device and steering side.

Positions are stored as normalized safe-area coordinates and clamped again
after rotation or window resizing. Phone/tablet and left/right-steering
profiles are independent. Touch visibility persists between launches. Controls
and Change Disc remain reachable even when gameplay controls are hidden. The
settings sheet also provides global control size and opacity choices without
shrinking the touch targets.

### Gas lock

Hold **Gas ✕** for two seconds to lock acceleration. CTRPad confirms the lock
with visual and haptic feedback. Tap Gas again to release it. The latch and all
other held inputs are released automatically when the editor opens, the
overlay is rebuilt or hidden, gameplay ends, or the app moves to the
background.

The overlay tracks touches independently, so steering and multiple buttons can
be held together. Empty overlay space passes through and does not create game
input.

## Controllers, keyboard, mouse, and fullscreen

Touch, keyboard, mouse buttons, and controller input are composed into the
same player-one PlayStation pad state. Connecting a controller does not rewire
the other mappings.

SDL's iOS controller path includes:

- both analog sticks and D-pad;
- Cross, Circle, Square, and Triangle;
- L1, L2, R1, and R2;
- Start and Select/Back;
- hot-plug events; and
- rumble when supported by the controller and operating system.

The software path and host input tests are present, but physical Bluetooth/MFi
pairing, reconnect behavior, latency, and rumble still need model-specific
device verification.

The desktop and iOS builds also accept these keyboard bindings:

| PlayStation input | Test layout | Original layout |
|---|---|---|
| D-pad | `W` `A` `S` `D` | Arrow keys |
| Triangle | `I` | `Z` |
| Square | `J` | `X` |
| Cross | `K` | `C` |
| Circle | `L` | `V` |
| L1 / R1 | `Q` / `E` | Left Shift / Right Shift |
| L2 / R2 | Left Ctrl / Right Ctrl | Left Ctrl / Right Ctrl |
| L3 / R3 | `[` / `]` | `[` / `]` |
| Start | `P` | Return |
| Select | Tab | Space |

Desktop mouse buttons provide convenient held actions without replacing
keyboard or controller steering:

| Mouse input | PlayStation input |
|---|---|
| Left button | Cross / Gas |
| Right button | Square / Brake |
| Middle button | Circle / Item |
| Mouse 4 | L1 / Drift |
| Mouse 5 | R1 / Drift |

Desktop windows are resizable and preserve the game presentation. Press `F11`
or `Alt-Return` (`Option-Return` on macOS) to enter or leave fullscreen.

## What works

| Area | Current result |
|---|---|
| Native game | CTR-ModSDK game source runs through CTRPad's host platform layer |
| Apple rendering | Native ARM64 macOS, iOS, and iPadOS presentation |
| Game setup | Files picker import and validated raw-disc loading |
| Touch | Analog steering, digital menu ring, face buttons, shoulders, Start, Select, persistent show/hide, editing, resizing, and Gas lock |
| Desktop input | Keyboard, mouse-button actions, controllers, resizable window, and fullscreen shortcuts |
| Controllers | SDL keyboard/gamepad composition and controller hot-plug path |
| Saves | Private memory-card persistence and non-destructive app updates |
| Lifecycle | Rotation, resizing, background/foreground, and held-input cleanup |
| Packaging | Retail-data exclusion, source identity, macOS ZIP/signing/notarization support, and unsigned or user-signed IPA output |
| App icon | Original macOS, iPhone, and iPad icon resources |

The full Apple-port evidence ledger, including what was tested only in
Simulator, lives under [`docs/parity/`](docs/parity/README.md). The historical
campaign record is under [`docs/history/`](docs/history/README.md).

## Desktop builds

### Windows with MSVC

Install Visual Studio 2022 or Build Tools 2022 with **Desktop development with
C++**, a current Windows SDK, and CMake. Then run:

```bat
build-msvc.bat
```

### Windows with MinGW

Install the MSYS2 i686 GCC, CMake, and Make packages, then run:

```bat
build.bat
```

### Linux

On Debian or Ubuntu:

```sh
sudo apt install gcc-multilib libx11-dev libxext-dev libgl1-mesa-dev \
  libasound2-dev libudev-dev libdbus-1-dev
chmod +x build.sh
./build.sh
```

SDL3 is vendored and linked statically. The first build compiles it from
source; subsequent builds reuse the selected build directory.

## Retail-free and reproducible packaging

The build does not read a disc image. Retail data enters only after
installation or through an explicit ignored development path.

Create the corresponding source archive from a clean committed checkout:

```sh
./package-source.sh
```

The source, macOS, and iOS packagers reject retail media, saves, runtime
containers, provisioning profiles, private-key-like files, and unrelated
binary packages.
Each Apple build records its full 40-character source commit. The packaging
workflow verifies that identity before producing release artifacts.

## Frequently asked questions

### Does this repository include Crash Team Racing?

No. Supply your own legally acquired compatible NTSC-U retail disc image. Do
not request or attach game downloads, extracted assets, or saves in issues.

### Can I move or resize the touch controls?

Yes. Open **Controls → Edit touch layout**. Layouts persist independently for
iPhone, iPad, left-hand steering, and right-hand steering.

### Can I hide the touch controls?

Yes. Open **Controls** and turn off **On-screen controls**. The Controls and
Change Disc utility buttons remain available, and the setting persists.

### Can Gas stay held without keeping my thumb down?

Yes. Hold Gas for two seconds to lock it, then tap it once to release it.

### Does CTRPad support Bluetooth controllers?

CTRPad retains SDL's native iOS game-controller path and full PS1-shaped
mapping alongside touch input. Physical behavior still depends on the
controller model, iOS version, and signing/device environment, so controller
pairing and rumble should be verified on real hardware.

### Is the Simulator result proof that it works on my iPhone or iPad?

No. Simulator is useful for builds, UI behavior, persistence, and automated
input checks. It does not prove physical touch ergonomics, haptics, Bluetooth,
performance, or device signing.

### Is there an App Store, TestFlight, or official paid build?

No. This repository documents local source builds and user-signed development
packages. CTRPad does not sell or distribute the game.

## Project map

| Path | Purpose |
|---|---|
| `main.c` | Process entry point and native platform boundary |
| `platform/` | Audio, input, storage, disc, rendering, lifecycle, and PSX facade glue |
| `game/` | CTR-ModSDK-derived game source used by the native build |
| `include/` | Native and game-facing declarations |
| `externals/SDL/` | Vendored SDL3 source |
| `platform/apple/` | macOS/iOS integration, property lists, touch UI, and asset catalog |
| `docs/INSTALL-MACOS.md` | macOS download, first launch, controls, and packaging guide |
| `docs/INSTALL-IOS.md` | iOS build, signing, sideload, and device acceptance guide |
| `docs/parity/` | Timestamped implementation and validation evidence |
| `docs/history/` | Apple-port campaign history and decisions |
| `package-ios.sh` | Retail-free unsigned or user-signed IPA creation |
| `package-macos.sh` | Retail-free ad-hoc or notarized macOS ZIP creation |
| `package-source.sh` | Deterministic corresponding-source archive |
| `tools/install-ios-simulator.sh` | Guarded single-Simulator update and launch |

Generated build trees, packages, game data, saves, credentials, and local
reference material must not be committed.

## Architecture

```text
main.c
  ├── platform/native_*       host audio, input, storage, disc, and rendering
  └── game/game_unity.h
        └── game/             decompiled game implementation
              └── include/    shared declarations and PS1-shaped interfaces
```

First-party native code targets portable C17. Platform-specific behavior stays
behind the native boundary, while keyboard, touch, and game controllers all
feed the same PlayStation-shaped input packets used by gameplay.

## Legal and acknowledgements

CTRPad is licensed under the [GNU General Public License v3](LICENSE). Each
third-party component retains its own license and copyright.

This project builds on:

- [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK), the decompilation
  project on which CTRPad is based;
- [PsyCross](https://github.com/OpenDriver2/PsyCross), from which portions of
  the owned native platform layer and PsyQ facade were derived; and
- [SDL3](https://github.com/libsdl-org/SDL), the cross-platform multimedia and
  controller layer.

Crash Team Racing and related names and marks belong to their respective
owners. The original CTRPad icon and touch overlay contain no extracted game
artwork, official logos, or PlayStation branding.
