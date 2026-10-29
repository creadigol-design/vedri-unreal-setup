# Brief: vedrí VP Toolkit (Unreal Engine plugin), revised for the Windows PC

> The owner's brief, kept verbatim as the source of requirements. This revision (2026-10-29) replaces
> the original Mac-authoring brief, which is still in git history. Decisions made since, and the places
> where the build deliberately departs from this text, are recorded in [PLAN.md](PLAN.md). Where they
> disagree, PLAN.md wins. Engine paths are placeholders until the owner fills them in. The PC session
> records the real paths in PLAN.md.

You are helping me build vedrí VP Toolkit, a C++ editor plugin for Unreal Engine that gets a virtual production project from "blank project" to "ready to shoot" in a few minutes instead of a few hours. I run vedrí Studio, a virtual production studio in North Wales. I'll be using this on real shoots, and I also want it clean enough to publish on Fab.

Work in plan mode first. Before writing any code, read this whole brief, inspect the installed engine source to confirm real class, plugin and setting names, then give me a short plan and ask me anything that's genuinely ambiguous. Don't guess Unreal API names — the VP plugins change between minor versions, so verify against the installed engines.

## Environment

* Machine: Windows 11 PC, Visual Studio 2022 with the Unreal workload, Windows SDK installed.
* Engines installed via the Epic launcher:
   * UE 5.5: `<<e.g. C:\Program Files\Epic Games\UE_5.5>>`
   * UE 5.6: `<<e.g. C:\Program Files\Epic Games\UE_5.6>>`
   * UE 5.7: `<<e.g. C:\Program Files\Epic Games\UE_5.7>>`
   * UE 5.8: `<<e.g. C:\Program Files\Epic Games\UE_5.8>>`
* Primary development target: UE 5.8. Do day-to-day work in a 5.8 test project (`<<path to a blank 5.8 Film/Video template project, or create one at C:\Dev\VPToolkitTest58>>`). 5.8 is the last planned 5.x release and ships the native Unreal MCP plugin (see "Using Unreal MCP" below). 5.5, 5.6 and 5.7 are compatibility targets you build and test headlessly.
* Launcher installs include headers and most plugin source under `<engine>\Engine\Plugins\` and `<engine>\Engine\Source\`. Read from there. My GitHub is linked to Epic, so if a launcher install is missing a `.cpp` you need, a sparse checkout of `github.com/EpicGames/UnrealEngine` at the matching release tag is allowed — keep it outside this repo.
* Hardware we use: Mo-Sys StarTracker Max (tracking), Blackmagic cameras and DeckLink cards. Must also support AJA because other studios use it.

## Hard constraints

1. Engine versions: must build and run on UE 5.5, 5.6, 5.7 and 5.8. Use `ENGINE_MAJOR_VERSION` / `ENGINE_MINOR_VERSION` guards for any API that differs. Never require engine source modifications. Don't design for UE6 yet, but keep version-specific code isolated so the 6.0 port is contained.
2. C++ editor plugin, module type `Editor`, loading phase correct for registering editor menus. Any Blueprint/content assets live in the plugin's `Content` folder. No dependency on project content.
3. Fab-ready: valid `.uplugin` with description, category, `Win64` in `PlatformAllowList`, `Resources/Icon128.png`, a `README.md`, a `CHANGELOG.md`, a licence file, no hardcoded paths, no third-party binaries we don't have rights to redistribute. Must pass `RunUAT BuildPlugin` cleanly for all four engine versions.
4. Idempotent and non-destructive: the wizard can be re-run. It never silently overwrites existing project settings or assets — it shows a diff/summary of what it will change and asks for confirmation. It writes config through the proper `UDeveloperSettings` / `UObject::SaveConfig` paths rather than hand-editing `.ini` files where possible.
5. Localisation-ready: every UI string goes through `LOCTEXT`. Ship English (`en`) and Welsh (`cy`) localisation. Welsh is not optional — it's part of the brand. Put all translatable strings in one place so I can review the Welsh.
6. No engine-level plugin install required: everything is a project plugin. Third-party plugins we rely on (tracking vendors, Blackmagic, AJA) are detected, not bundled.
7. Clean module boundaries: each `.Build.cs` lists exactly the modules it needs. Optional integrations (each tracking vendor, each card family, nDisplay, the 5.8 MCP toolset) live behind small provider classes with a no-op fallback, so a missing plugin never stops the toolkit loading.

## What it does

Add a toolbar button and a `Tools → vedrí VP Toolkit` menu entry that opens a Setup Wizard (Slate, modal window, multi-step with back/next/apply). First screen has two radio buttons:

* Set up Green Screen shoot
* Set up LED shoot

Each subsequent step is a group of checkboxes with sensible defaults for the chosen mode. Everything below is a step unless marked otherwise.

### Step 1 — Enable required plugins

Enable (and prompt for the editor restart that needs) the engine plugins the chosen mode requires. Verify the exact plugin names against each engine version; expected set:

* Both modes: Media Framework Utilities, Media IO Framework, Live Link, Live Link FreeD, Live Link Lens, Live Link XR, Live Link VRPN, Camera Calibration, Lens Distortion, Timed Data Monitor, Remote Control API, Virtual Production Utilities, Takes Recorder, OpenColorIO, Switchboard
* 5.8 only: Live Link Hub integration where it changes the setup (it went production-ready in 5.8 — check what, if anything, the toolkit should configure for it)
* Video I/O: Blackmagic Media Player and/or AJA Media Player depending on the card chosen in Step 2
* LED mode only: nDisplay, nDisplay Support, ICVFX / In-Camera VFX, Live Link Over nDisplay, Multi-User Editing, light-card utilities
* Green screen mode only: nothing extra (we do not key in Unreal — see Step 5)
* Third-party tracking plugins: detect which are installed (see Step 6) and enable the ones the user picks; show a notice with the vendor download link for any the user wants but doesn't have.

### Step 2 — Video I/O card

Radio: Blackmagic DeckLink / AJA. Auto-detect which SDK plugin is enabled and which devices are present, pre-select, allow override. Everything downstream (timecode provider, custom time step, media sources, media outputs) is built for the chosen card family. Adding a third card family later must be one new provider class, not a rewrite.

### Step 3 — Timing: frame rate, genlock, timecode

* Frame rate presets with UK broadcast defaults: default 25 fps, with 50, 24, 23.976, 29.97, 30, 59.94, 60 selectable. Resolution presets: 1920×1080 default, 3840×2160 selectable, plus custom.
* Set the project's fixed frame rate, disable smooth frame rate, set the engine's Custom Time Step to the card's genlock time step class (Blackmagic or AJA), and set the Timecode Provider to the card's timecode provider class. Let the user pick the device/port and whether genlock is from the SDI input or reference input.
* Create a Media Profile asset (in `/Game/VPToolkit/Config/`) that captures: timecode provider, custom time step, media source proxies, media output proxies. Set it as the project's active Media Profile. Explain in a comment why we use Media Profiles rather than raw engine settings.
* Add a Timing health panel (non-modal dockable tab, also reachable from the menu) that shows live: genlock state (locked / not locked / no signal), timecode provider state and current timecode, engine frame rate vs. target, and each Live Link tracking subject's status. Use the Timed Data Monitor where it makes sense instead of reinventing it.

### Step 4 — Video Out

* Create a Media Output asset for the chosen card (SDI output, format matching Step 3, 10-bit YUV default with RGB/8-bit as options, key+fill option where the card supports it).
* Create a Media Capture setup that outputs the active viewport (or a chosen Cine Camera) to that SDI output, with a one-click Start / Stop Output button in the health panel and on the toolbar.
* Set the output to respect the genlock so frames leave on the clock.

### Step 5 — Mode-specific setup

#### Green screen mode

* Create a Media Source for the camera plate (card input matching Step 3) and a Media Plate / Media Bundle so the live plate can be viewed in-editor for line-up.
* Do not set up Composure or any chroma keying. Our workflow is: camera plate goes to an external keyer/switcher; Unreal sends clean CG out over SDI (Step 4). Document this clearly in the generated README for the project. (Note: 5.8 introduces a new compositing framework and deprecates some Composure-based workflows — don't build on either; just make sure the plate-in / CG-out path works on all four versions.)
* Optionally (checkbox) create a second Media Output for a matte/alpha or key feed if the card supports it, so the external keyer has what it needs.
* Create a `BP_VP_GreenScreen_Stage` level with: the tracked Cine Camera (Step 6), a floor-plane reference at real-world scale, and a placeholder background. Spawn nothing heavy.

#### LED mode

* Show a Wall specs form: wall width (m), wall height (m), pixel pitch (mm), panel/tile size (mm), number of render nodes, optional ceiling (w × d, pitch). Defaults pre-filled to our planned wall: 8 m × 3 m. Derive wall resolution from dimensions and pitch and show it live.
* From the form, generate an nDisplay Config asset (`/Game/VPToolkit/nDisplay/`): a cluster with the specified node count, viewports split evenly across nodes, a mesh/screen matching the wall geometry (flat by default; curvature radius optional), and an ICVFX camera component with sensible inner-frustum defaults (resolution, overscan, soft edge). Also add default light cards and chromakey markers settings so people can tweak rather than build from scratch.
* Generate a Switchboard config stub pointing at the nDisplay config, with placeholder node names/IPs and a note on how to fill them in.
* Create a `BP_VP_LED_Stage` level containing the nDisplay root actor, the tracked Cine Camera, and a simple gray-box set at real scale.
* Apply the known nDisplay-friendly project settings (verify each against the engine): disable VSync, appropriate texture streaming settings, multi-GPU opt-in if more than one GPU is detected, and anything else the ICVFX quick-start docs recommend.

### Step 6 — Camera tracking and lens

Tracking is a provider list, not a single choice. The wizard shows every tracking source it knows about, marks which are available (engine built-in, or third-party plugin detected in the project/engine), and lets the user pick one as the primary camera source. Each provider knows how to create and configure its Live Link source and what subject/role it exposes. Use each vendor's documented defaults for ports and protocols — read their plugin source or docs; don't invent port numbers.

Built into the engine (always available):

* FreeD (Live Link FreeD) — the universal fallback; works with Mo-Sys, stYpe, Ncam, EZtrack, ZEISS, Vinten, Panasonic/Sony PTZ and most others. Configurable UDP port, subject name, and the FreeD encoder/zoom/focus mapping options the engine exposes.
* Live Link XR / OpenXR — Vive trackers and similar, for budget or rehearsal setups.
* Live Link VRPN — generic.
* Live Link Hub (5.8+) — if the user tracks through Hub rather than direct sources.

Third-party, detected by plugin name and configured if present (ship a provider for each; it degrades to a "plugin not installed — get it here" notice if absent):

* Mo-Sys StarTracker via the Mo-Sys Live Link plugin — our primary system.
* ZEISS CinCraft Scenario via the free ZEISS CinCraft UE Connect plugin from Fab (Live Link source per CinCraft data stream; it supports 5.8). Configure its Live Link source and lens stream; fall back to FreeD if the plugin isn't present.
* stYpe (stYpe plugin), Ncam (Ncam AR Suite), OptiTrack (Motive Live Link), Vicon (Vicon Live Link), HTC Vive Mars — each as a thin provider. If a vendor's plugin API is too opaque to configure reliably, the provider can simply enable the plugin, create the Live Link source with defaults, and tell the user what to fill in manually — but it must still be a provider so the wizard UI is consistent.

Then, for the chosen primary source:

* Create a Live Link preset asset with the source and subject pre-configured, and set it to auto-apply on editor start.
* Create a `BP_VP_TrackedCineCamera` actor: Cine Camera + Live Link Controller driving transform (and FIZ where the source supplies it) + Lens component referencing a Lens File asset. Create an empty Lens File with a note that calibration is done through the Camera Calibration plugin's tools; don't try to calibrate. Where a vendor supplies lens data directly (ZEISS, Mo-Sys), wire that path and say so in the generated README.
* Sensor defaults: let the user pick from a short list of common cameras (include Blackmagic Pyxis 6K / URSA sensor sizes) or enter custom sensor dimensions.

### Step 7 — Project settings hardening (both modes)

A reviewed checklist of settings the wizard will apply, each with a checkbox and a one-line reason shown in the UI. Include, after verifying against the engine: fixed frame rate, smooth frame rate off, motion blur off by default, temporal upsampling considerations, "use less CPU when in background" off, default map set to the generated stage level, OpenColorIO config placeholder, Remote Control preset stub, Takes Recorder defaults (timecode-stamped take names).

### Presets

* Save / Load wizard presets as JSON (`Saved/VPToolkit/Presets/*.json`) so a studio can keep "Our 8m wall, 2 nodes, Blackmagic, 25p, Mo-Sys" and reapply it to a new project in one click. Ship bundled presets: `vedri-greenscreen-25p-blackmagic-mosys` and `vedri-led-8x3m-25p-blackmagic-mosys`, plus generic `greenscreen-freed` and `led-freed` variants.

### Summary / Apply

Final step shows everything that will be created or changed, grouped by plugins / settings / assets, with a per-item tick. On Apply: do the work, log each action to the Output Log under a `LogVPToolkit` category, write a `VPToolkit_SetupReport.md` into the project's `Saved/VPToolkit/` folder, and offer to restart the editor if plugin enablement requires it.

### 5.8 only — expose the toolkit as an MCP toolset

On 5.8, register the toolkit's setup actions as a custom toolset in Epic's Toolset Registry, so that any MCP client connected to the editor (Claude Code, Cursor, etc.) can run `vp_toolkit.setup_shoot(mode="led", wall_width_m=8, wall_height_m=3, nodes=2, card="blackmagic", fps=25, tracking="mosys")`, `vp_toolkit.preview_changes(...)`, `vp_toolkit.get_health()`, `vp_toolkit.list_presets()`, `vp_toolkit.apply_preset(name)`. This is the same Preview/Apply action layer the wizard uses — no second code path. Gate it behind `ENGINE_MINOR_VERSION >= 8` and the presence of the ToolsetRegistry module, as its own module (`VedriVPToolkitMCP`) so 5.5–5.7 builds never touch it. Read Epic's ToolsetRegistry source in the 5.8 install to see how official toolsets register themselves and copy that pattern. This is a Fab differentiator: "set up your stage from a chat prompt".

### Using Unreal MCP during development (5.8)

Epic's native MCP server lets you drive and inspect the running 5.8 editor from Claude Code. Use it as your test loop instead of asking me to click through the editor. Setup in the 5.8 test project:

1. Enable plugins Unreal MCP, All Toolsets (and Toolset Registry, which comes as a dependency); restart.
2. `Edit → Editor Preferences → General → Model Context Protocol`: turn on Auto Start Server (default port 8000, path `/mcp`, so `http://127.0.0.1:8000/mcp`).
3. In the editor console run `ModelContextProtocol.GenerateClientConfig ClaudeCode` — it writes a `.mcp.json` into the project root. Launch Claude Code from that project root so it's picked up.

Then, for every milestone: run the toolkit's action (via the wizard or, once it exists, via the toolkit's own MCP toolset), and use Epic's tools to read back the actual editor state — enabled plugins, project settings values, assets created under `/Game/VPToolkit/`, Live Link sources and subjects, Media Profile contents, nDisplay config structure — and compare against what the action's Preview promised. Take editor screenshots when the UI matters. Run the Automation tests through the MCP test tools. Note the limits: it's experimental, loopback-only, no auth, and the API may change — never ship anything that depends on it, and keep all MCP use out of the plugin's own code except the gated 5.8 toolset module above.

For 5.5–5.7 there is no native MCP. Verify those with `RunUAT BuildPlugin`, headless Automation tests (`UnrealEditor-Cmd <project> -ExecCmds="Automation RunTests VPToolkit" -nullrhi -unattended -nopause -testexit="Automation Test Queue Empty"`), and the Python Editor Script Plugin's remote execution if you need to inspect editor state (the `runreal/unreal-mcp` server speaks that protocol and works on 5.4+ — acceptable as a dev-only tool).

## Architecture

Suggested, adjust if you find a better fit after reading the engine:

```
VedriVPToolkit/
  VedriVPToolkit.uplugin
  Resources/Icon128.png
  Content/            # Blueprint actors, bundled presets, starter levels
  Config/             # Default settings, localisation
  Source/
    VedriVPToolkit/           # Runtime-safe data types, presets, settings (minimal)
      Private/Core/           # Engine-independent logic: wall maths, preset schema, diff model
    VedriVPToolkitEditor/     # Wizard UI, menu/toolbar, setup actions
      Private/Wizard/         # Slate steps, one class per step
      Private/Actions/        # One class per setup action (EnablePlugins, ConfigureTiming, CreateMediaProfile, GenerateNDisplayConfig, ...) implementing Preview() -> list of changes, Apply(), Validate()
      Private/IO/             # IVPVideoIOProvider: Blackmagic, AJA
      Private/Tracking/       # IVPTrackingProvider: FreeD, LiveLinkXR, VRPN, LiveLinkHub, MoSys, ZeissCinCraft, Stype, Ncam, OptiTrack, Vicon, ViveMars
      Private/Health/         # Dockable timing/health tab
    VedriVPToolkitMCP/        # 5.8-only Toolset Registry integration (whole module guarded)
  Tests/                      # Automation tests
  Scripts/
    BuildPlugin.ps1           # RunUAT BuildPlugin for 5.5, 5.6, 5.7, 5.8
    RunTests.ps1              # headless Automation run per engine version
  Docs/
    README.md, CHANGELOG.md, LICENSE, USER_GUIDE.md, TEST_PLAN.md, TRACKING_PROVIDERS.md
```

Key design rules: every setup action is a small class with Preview / Apply / Validate, and every integration (card, tracker, nDisplay, MCP) is a provider with a no-op fallback. The wizard and the MCP toolset just compose actions. That's what makes it idempotent, testable, and easy to extend.

## Quality bar

* Compiles with zero warnings on all four engine versions. `Scripts/BuildPlugin.ps1` is the proof and runs at the end of every milestone.
* Automation tests for: preset round-trip, nDisplay resolution/viewport derivation from wall specs, action Preview returning correct diffs against a known project state, version guards, each tracking provider reporting available/unavailable correctly, and the MCP toolset (5.8) returning the same results as the wizard for the same inputs.
* A `CLAUDE.md` in the repo root capturing the conventions you settle on (naming, where actions and providers live, how to add a card family or tracking source, how to run the four-version build and tests), so future sessions stay consistent.
* `Docs/TEST_PLAN.md`: the manual on-set checks for the things no test can cover — DeckLink/AJA genlock lock, timecode on the SDI output, StarTracker subject moving the camera, nDisplay on real nodes — written so I can hand them to a freelancer.
* Commit in small, buildable steps with clear messages.

## How I want to work

1. Plan first. Read the Blackmagic, AJA, nDisplay, Live Link (incl. FreeD, XR, VRPN, Hub), MediaFrameworkUtilities, CameraCalibration and (5.8) ToolsetRegistry modules in the installed engines and tell me which class names / settings you'll actually use, with the 5.5 → 5.8 differences you found. Also check Fab/vendor sites for the current third-party tracking plugin names and which engine versions each supports, and put that in `Docs/TRACKING_PROVIDERS.md`.
2. Ask me before: choosing between two reasonable architectures, anything that touches licensing/redistribution, and anything in the nDisplay generation you're not confident about.
3. Build in milestones. Each ends with a clean four-version build, passing tests, and a read-back through MCP on 5.8:
   * M1: plugin skeleton, `.uplugin`, `.Build.cs` files, menu/toolbar, wizard shell with mode selection, plugin-enablement action, build + test scripts, MCP dev loop working.
   * M2: presets (schema, save/load, bundled), project-settings hardening actions, Summary/Apply flow with diff preview and setup report.
   * M3: tracking provider framework + built-in providers (FreeD, XR, VRPN, Hub) + Mo-Sys + ZEISS CinCraft; remaining vendors as thin providers; Lens File; `BP_VP_TrackedCineCamera`.
   * M4: timing + Video Out + green-screen mode — card-family providers, Media Profile, health panel, Start/Stop Output.
   * M5: LED mode — wall-specs form, resolution maths, nDisplay config generation, Switchboard stub, LED stage level.
   * M6: 5.8 MCP toolset module.
   * M7: localisation (en + cy), docs, Fab packaging, final test pass on all four versions, `TEST_PLAN.md`.
4. After each milestone, tell me in a few lines what to check on the real kit (DeckLink + StarTracker) that you couldn't verify yourself.

Start with the plan.
