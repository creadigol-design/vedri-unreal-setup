# vedrí VP Toolkit

An Unreal Engine editor plugin that takes a virtual production project from a blank project to ready
to shoot in minutes. A setup wizard configures plugins, genlock and timecode, SDI video in and out,
camera tracking and lens, and either a green-screen stage (clean CG out to an external keyer) or an LED
stage (nDisplay cluster generated from the wall's dimensions). Every change is previewed and confirmed
before it's applied, and the wizard is safe to re-run.

Made by vedrí studio, North Wales. Targets Unreal Engine 5.5, 5.6 and the
newest 5.x on Windows (Win64). Supports Blackmagic DeckLink and AJA cards, and Mo-Sys StarTracker or
generic FreeD tracking.

## Status

Early development. The engine-independent core is in place and tested: LED wall resolution and
render-node split, frame-rate and SDI checks, preset format, validation, and the change preview / setup
report model. The Unreal-facing plugin (wizard, menus, card and tracking integration) comes next. See
[docs/PLAN.md](docs/PLAN.md) for milestones and [docs/WINDOWS_TODO.md](docs/WINDOWS_TODO.md) for what
still has to be proven on the Windows PC.

## Layout

| Path | What |
|---|---|
| `VedriVPToolkit/` | The plugin (what gets packaged for Fab) |
| `VedriVPToolkit/Source/VedriVPToolkitEditor/Private/Core/` | Engine-independent core, header-only |
| `VedriVPToolkit/Resources/Presets/` | Bundled presets (`vedri-greenscreen-25p-blackmagic`, `vedri-led-8x3m-25p-blackmagic`) |
| `Tests/Standalone/` | Core tests that build without Unreal |
| `docs/` | Brief, plan, Windows TODO, test plan, validation-code catalogue |

## Running the core tests

Requires a C++20 compiler (Xcode command line tools on macOS) and `python3`:

```bash
make -C Tests/Standalone
```

See [docs/TEST_PLAN.md](docs/TEST_PLAN.md) for the full procedure.

## Licence

Proprietary. Copyright © 2026 vedrí studio Ltd. All rights reserved. See [LICENSE](LICENSE).
