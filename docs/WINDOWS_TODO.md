# Windows TODO: what is unproven

This is the honest list of everything not yet checked against a real engine build, the real kit
(DeckLink/AJA, StarTracker, render nodes), or a spec sheet. Every `TODO(verify-on-kit)` and
`TODO(confirm-spec)` in the source must appear here. Remove an entry once it's verified and say how in
the commit message.

## Engine API

No engine-facing code has been written yet (decision D1 in [PLAN.md](PLAN.md)). Entries get added here as
M1–M7 write code that can't be fully proven by a build, Automation tests or the 5.8 MCP read-back.

## Core layer on MSVC

The core (`VedriVPToolkitEditor/Public/Core/*.h`) passes with clang and gcc under `-Wall -Wextra
-Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion`, with `-fno-exceptions -fno-rtti`, and with
the Unreal/Windows macro poison list. It has not been compiled by MSVC yet.

- [ ] First `BuildPlugin` compiles the Core headers with zero warnings on 5.5, 5.6, 5.7 and 5.8.
- [ ] The plugin builds as C++20 on all four versions. Core uses defaulted `operator==`. If any version builds plugins as C++17, add explicit comparisons.
- [ ] Unity builds: the Core headers are `inline` only, with no anonymous namespaces or `static` free functions, so they should be safe. Confirm, and check that `bUseUnity=false` (IWYU) also builds.

## Packaging

- [ ] `RunUAT BuildPlugin` includes `Resources/Presets/*.json` in the packaged plugin. If it doesn't, add a `Config/FilterPlugin.ini` entry.

## Tracking providers

Every row in [TRACKING_PROVIDERS.md](TRACKING_PROVIDERS.md) is unverified. Confirm the plugin names, Live
Link source classes and default ports against installed plugins before a provider relies on them,
starting with the free Mo-Sys Live Link plugin (D5).

## Data to confirm

| Item | Where | Current value | Check against |
|---|---|---|---|
| Camera sensor sizes | `Core/VPCoreCameraSensors.h` | PYXIS 6K 36 × 24; URSA Cine 12K LF 35.64 × 23.32; URSA Mini Pro 12K 27.03 × 14.25; URSA Mini Pro 4.6K G2 25.34 × 14.25; Pocket 6K Pro 23.10 × 12.99; ALEXA 35 27.99 × 19.22; VENICE 2 8.6K 35.9 × 24.0 (mm) | Current manufacturer spec sheets |
| Default genlock source | `Core/VPCoreWizardConfig.h` (`VideoIOConfig`) | SDI input | What each card family's custom time step supports (SDI vs reference) |
| Default connectors | `Core/VPCoreWizardConfig.h` | input 1, output 2, matte 3 | How the Blackmagic and AJA plugins number connectors |
| LED panel | `Core/VPCoreWallLayout.h` (`PanelSpec`) | 2.84 mm pitch, 500 × 500 mm tile (176 px) | The chosen panel's spec sheet |
| GPU window limit | `Core/VPCoreWallLayout.h` (`MaxWindowDimension`) | 16384 px | Render node GPUs |

## nDisplay geometry (M5)

- [ ] Map Core's stage frame (right-handed, metres: Right / Up / TowardStage) to Unreal's frame (left-handed, centimetres, X forward / Y right / Z up) and to the nDisplay root's orientation.
- [ ] Confirm mesh winding and UV conventions. Core emits counter-clockwise triangles as seen from the stage, with UV (0,0) at the image's top-left. Check these match nDisplay mesh projection, or flip them in the conversion.
- [ ] Ceiling image orientation matches how the LED processor maps the ceiling.
