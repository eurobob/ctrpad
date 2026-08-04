# Build and Run CTRPad on visionOS

CTRPad's visionOS target is a development preview. It contains no Crash Team
Racing data, PlayStation BIOS, certificate, provisioning profile, or private
key. Supply your own compatible NTSC-U raw MODE2/2352 disc image and sign the
app with your own Apple development team.

## What the target provides

The app exposes three presentations over the same running game:

| Mode | Presentation | Camera behavior |
|---|---|---|
| Window | SwiftUI window with a Metal view | Original 4:3 mono camera; launcher and fallback |
| Portal | Mixed immersive Compositor Services layer | Two chase cameras with physical IPD, head translation, and off-axis convergence behind a world-fixed 4:3 opening |
| Cockpit VR | Full immersive Compositor Services layer | CTR's first-person kart camera plus per-eye position and headset-relative pitch, yaw, and roll |

The game simulation, audio, menu input, and HUD logic still run once per
frame. Only world projection and world primitive emission run once per eye.
The HUD is emitted into a third transparent layer and composited over both
eyes. This avoids advancing pickups, lap state, menus, or sound twice merely
because stereo is active.

## Requirements

- an Apple Silicon Mac;
- current Xcode with the visionOS SDK and Simulator installed;
- CMake 3.28 or newer and Ninja;
- for a physical headset, a development identity/profile that authorizes your
  chosen bundle identifier and Apple Vision Pro; and
- your own compatible `SCUS_944.26` single-track raw MODE2/2352 BIN.

The visionOS sources use SwiftUI immersive spaces, ARKit world tracking,
Compositor Services, and Metal vertex amplification. They cannot be compiled
or device-validated by a Linux build.

## Build the Simulator app

```sh
brew install cmake ninja
cmake --preset visionos-simulator-arm64
cmake --build --preset visionos-simulator-arm64
open build-visionos-simulator-arm64/CTRPad.app
```

For an Xcode project with a selectable Apple Vision destination, configure the
same target with the Xcode generator:

```sh
cmake -S . -B build-visionos-xcode -G Xcode \
  -DCMAKE_SYSTEM_NAME=visionOS \
  -DCMAKE_OSX_SYSROOT=xrsimulator \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=2.0 \
  -DCTR_NATIVE_RENDERER_GLES=OFF
open build-visionos-xcode/CTR-Native.xcodeproj
```

Select an Apple Vision Pro Simulator destination and run the `ctr_native`
target.

The Simulator is useful for the launcher, import flow, compositor lifecycle,
and basic stereo inspection. It is not proof of headset comfort, tracking
latency, controller behavior, or sustained frame cadence.

## Build for Apple Vision Pro

Choose a unique App ID when necessary:

```sh
cmake --preset visionos-device-arm64 \
  -DCTR_NATIVE_VISIONOS_BUNDLE_IDENTIFIER=com.example.yourname.ctrpad.vision
cmake --build --preset visionos-device-arm64
```

The result is `build-visionos-device-arm64/CTRPad.app`. To manage development
teams and signing through Xcode, repeat the Xcode-generator command above with
`CMAKE_OSX_SYSROOT=xros`, then select the `ctr_native` target's Signing &
Capabilities settings. Do not commit profiles, certificates, private keys, a
signed bundle, or retail game data.

The repository also includes a small root `CTRPad.xcodeproj` adapter for remote
build and device services such as Mesa. Build or run its `CTRPad` scheme. The
adapter delegates the native build to the same `visionos-device-arm64` CMake
preset and lets Xcode perform development signing and installation; it does not
replace the CMake build or compile a second game implementation. Its XcodeGen
source remains in `project.yml` for maintainers who need to regenerate it.

On first launch:

1. Select **Choose Disc…** in the launcher window.
2. Pick the raw NTSC-U BIN through the system file importer.
3. CTRPad copies it into its private Application Support container, validates
   it, and starts the native runtime on a dedicated game thread.
4. Connect a controller and choose **Portal** or **Cockpit VR**.
5. Use **Exit** or the system immersive-space control to return to the window.

The imported image persists with the app container. Delete the app only if you
also intend to delete that private copy and its saves.

## Stereo implementation notes

`platform/native_vision.c` is the boundary between predicted compositor poses
and CTR's camera/push-buffer code. It publishes a coherent pair of eye poses,
temporarily applies each pose to player one's camera, and assigns the resulting
ordering tables to left, right, and HUD layers. The allocation reserves all
three layers at level startup, so entering an immersive space during a race
does not require reallocating retail memory structures.

`platform/visionos/native_renderer_vision.m` is a Metal implementation of the
native renderer API. It decodes CTR's PS1 VRAM texture formats and publishes a
completed two-slice scene texture plus the transparent HUD texture through
`CTRVisionFrameHub`. `CTRCompositorRenderer.swift` predicts the device anchor
for the drawable presentation time and then presents those textures either on
the fixed portal plane or across the full cockpit view.

The media-free stereo math check is available on any desktop build:

```sh
./ctr_native --self-test-vision-stereo
```

## Development-preview limits

- Stereo is currently a single-player presentation. Split-screen gameplay
  remains on the normal mono window path.
- The first Metal backend covers the normal PS1 world, HUD, VRAM texture,
  blending, mask, and dither paths. Framebuffer-to-VRAM feedback, offscreen
  render-target round trips, and native override textures still need parity
  work, so heat-haze, screen-copy, and video-style effects can differ from the
  established OpenGL renderer.
- The source and media-free checks have been validated off-device; the Swift,
  Compositor Services, signing, comfort, and performance acceptance steps need
  a Mac with the visionOS SDK and, for release confidence, a physical headset.

## Physical-headset acceptance checklist

Before calling a build release-ready, verify on an actual headset:

- left/right eye assignment and comfortable convergence;
- portal parallax when leaning left, right, up, and forward;
- cockpit yaw/pitch/roll signs and horizon stability;
- HUD legibility and black fade transitions in both modes;
- race start, items, rain, turbo, pause, end-of-race, and level transitions;
- controller reconnect and application background/foreground behavior; and
- sustained compositor cadence and thermal behavior for at least one complete
  race at the intended internal resolution.

If an eye axis or world scale needs calibration, change the compositor's
published pose only. Do not add per-eye gameplay ticks or duplicate audio as a
rendering workaround.
