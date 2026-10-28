# Windows TODO: what is unproven

This is the honest list of everything that hasn't been checked against a real engine, compiler or
hardware. Every `TODO(verify-on-pc)` and `TODO(confirm-spec)` in the source must appear here. Work through
it in M7, when the PC arrives.

## Engine API

No engine-facing code has been written yet (decision D1 in [PLAN.md](PLAN.md)), so there are no engine
calls to verify yet. Engine calls get added here as M1–M6 write them.

## Core layer on MSVC

The core (`VedriVPToolkitEditor/Private/Core/*.h`) passes with clang and gcc under `-Wall -Wextra
-Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion`, with `-fno-exceptions -fno-rtti`, and with
the Unreal/Windows macro poison list. It has not been compiled by MSVC yet.

- [ ] First Win64 build compiles the Core headers with zero warnings on 5.5, 5.6 and the newest 5.x.
- [ ] Check the plugin builds as C++20 on all three versions. Core uses defaulted `operator==`. If an engine version builds plugins as C++17, add explicit comparisons.
- [ ] Unity builds: the Core headers are `inline` only, with no anonymous namespaces or `static` free functions, so they should be safe. Confirm, and check that `bUseUnity=false` (IWYU) also builds.

## Packaging

- [ ] `RunUAT BuildPlugin` includes `Resources/Presets/*.json` in the packaged plugin. If it doesn't, add a `Config/FilterPlugin.ini` entry.

## Data to confirm

| Item | Where | Current value | Check against |
|---|---|---|---|
| Camera sensor sizes | `Core/VPCoreCameraSensors.h` | PYXIS 6K 36 × 24; URSA Cine 12K LF 35.64 × 23.32; URSA Mini Pro 12K 27.03 × 14.25; URSA Mini Pro 4.6K G2 25.34 × 14.25; Pocket 6K Pro 23.10 × 12.99; ALEXA 35 27.99 × 19.22; VENICE 2 8.6K 35.9 × 24.0 (mm) | Current manufacturer spec sheets |
| FreeD UDP port default | `Core/VPCoreWizardConfig.h` (`TrackingConfig`) | 40000 | The engine's FreeD source default, and the StarTracker's FreeD output setting |
| Default genlock source | `Core/VPCoreWizardConfig.h` (`VideoIOConfig`) | SDI input | What each card family's custom time step actually supports (SDI vs reference) |
| Default connectors | `Core/VPCoreWizardConfig.h` | input 1, output 2, matte 3 | How the Blackmagic and AJA plugins number DeckLink / AJA connectors |
| LED panel | `Core/VPCoreWallLayout.h` (`PanelSpec`) | 2.84 mm pitch, 500 × 500 mm tile (176 px) | The chosen panel's spec sheet |
| GPU window limit | `Core/VPCoreWallLayout.h` (`MaxWindowDimension`) | 16384 px | Render node GPUs |

## nDisplay geometry (M5)

- [ ] Map Core's stage frame (right-handed, metres: Right / Up / TowardStage) to Unreal's frame (left-handed, centimetres, X forward / Y right / Z up) and to the nDisplay root's orientation.
- [ ] Confirm mesh winding and UV conventions. Core emits counter-clockwise triangles as seen from the stage, with UV (0,0) at the image's top-left. Check these match nDisplay mesh projection, or flip them in the conversion.
- [ ] Ceiling image orientation matches how the LED processor maps the ceiling.
