# Brief: vedrí VP Toolkit (Unreal Engine plugin)

> The original brief, kept verbatim as the source of requirements. Decisions made since (and the places
> where the build deliberately departs from this text) are recorded in [PLAN.md](PLAN.md); where they
> disagree, PLAN.md wins.

You are helping me build vedrí VP Toolkit, a C++ editor plugin for Unreal Engine that gets a virtual production project from "blank project" to "ready to shoot" in a few minutes instead of a few hours. I run vedrí Studio, a virtual production studio in North Wales. I'll be using this on real shoots, and I also want it clean enough to publish on Fab.

Work in plan mode first. Before writing any code, read this whole brief, inspect the engine source at the path below to confirm real class and setting names, then give me a short plan and ask me anything that's genuinely ambiguous. Don't guess Unreal API names — the VP plugins change between minor versions, so verify against the installed engine.

## Environment

* Authoring machine: macOS. Unreal Engine is NOT installed here and won't be. You are writing code that will be compiled, run and tested for the first time on a Windows PC later. Nothing in this repo should assume an engine on this machine.
* Build and shipping target: Windows 10/11, Win64, Visual Studio 2022. That's where the SDI cards, nDisplay and the shoots live. The PC isn't available yet; treat first compile as a later milestone.
* Hardware we use: Mo-Sys StarTracker Max (tracking), Blackmagic cameras and DeckLink cards. Must also support AJA because other studios use it.
* Epic GitHub access for engine source: yes, my GitHub account is linked to Epic, so `github.com/EpicGames/UnrealEngine` is cloneable from this machine.

## Working without an engine on this machine

Because you can't compile here, correctness has to come from reading real source, not memory. Rules:

1. Get the real headers first. Do a sparse, blobless checkout of `github.com/EpicGames/UnrealEngine` into `../ue-ref/<version>` for the `5.5` release tag, the `5.6` tag, and the latest released `5.x` tag (check the repo's tags — don't assume which is newest). Pull only what you need: `Engine/Plugins/Media/BlackmagicMedia`, `Engine/Plugins/Media/AjaMedia`, `Engine/Plugins/Media/MediaFrameworkUtilities`, `Engine/Plugins/Media/MediaIOFramework`, `Engine/Plugins/Runtime/nDisplay`, `Engine/Plugins/VirtualProduction`, `Engine/Plugins/Animation/LiveLink`, `Engine/Plugins/Runtime/LiveLinkFreeD`, `Engine/Plugins/Compositing/OpenColorIO` (confirm exact paths from the tree), plus `Engine/Source/Runtime/Engine/Classes/Engine/` (`Engine.h`, `TimecodeProvider.h`, `EngineCustomTimeStep.h`), `Engine/Source/Runtime/Engine/Classes/GameFramework/` (project/general settings), `Engine/Source/Developer/ToolMenus`, and `Engine/Source/Runtime/Projects` (plugin manager, for enabling plugins). Use `git clone --filter=blob:none --sparse` then `git sparse-checkout set ...` so it stays small. Keep `ue-ref/` out of this repo (add to `.gitignore` if it ends up inside). If the clone fails with a 404/403 it means the Epic–GitHub link isn't active on this machine's git credentials — stop and tell me rather than falling back to memory.
2. Every engine API you call must be traceable to a header you've read. Where 5.5 and a later version differ, use `ENGINE_MINOR_VERSION` guards and note both in a comment. Where you couldn't verify something, say so in a `// TODO(verify-on-pc): ...` comment and list it in `docs/WINDOWS_TODO.md`. Never leave an unverified call unmarked — a silent guess costs me a day on the PC.
3. Keep engine-independent logic engine-independent. Put pure logic — nDisplay resolution/viewport maths from wall specs, preset JSON schema and validation, the Summary/Apply diff model — in plain C++ headers with no `UObject` dependency, under `Source/VedriVPToolkit/Private/Core/`. Write small standalone tests for those in `Tests/Standalone/` that build with `clang++` on this Mac via a `Makefile` or `CMakeLists.txt`, so at least that layer is proven before the PC. Unreal Automation tests wrap the same logic on the PC.
4. Be strict about module boundaries. Each `.Build.cs` lists exactly the modules it needs, nothing more, and Windows-only ones (`BlackmagicMedia`, `AjaMedia`, `DisplayCluster`, `DisplayClusterConfiguration`, etc.) are added only when `Target.Platform == UnrealTargetPlatform.Win64` (or Linux) behind a define such as `VPTOOLKIT_WITH_VIDEOIO` / `VPTOOLKIT_WITH_NDISPLAY`. The card-family and nDisplay classes have a no-op fallback so the plugin still loads where those modules are absent.
5. Review passes instead of compiles. At the end of every milestone, do a dedicated review pass over the new code checking: includes vs. module dependencies, `UCLASS`/`UPROPERTY` macro correctness, generated-header include is last, `LOCTEXT_NAMESPACE` defined/undefined per file, `.Build.cs` consistency, and that every engine call matches the header you read. Write the findings and fixes into the commit message.
6. `PlatformAllowList` in the `.uplugin`: `Win64` only.

## Hard constraints

1. Engine versions: must build and run on UE 5.5, 5.6 and the latest released 5.x. Use `ENGINE_MAJOR_VERSION` / `ENGINE_MINOR_VERSION` guards for any API that differs. Never require engine source modifications.
2. C++ editor plugin, module type `Editor`, loading phase `PostEngineInit` (or whatever is correct for registering editor menus). Any Blueprint/content assets live in the plugin's `Content` folder. No dependency on project content.
3. Fab-ready: valid `.uplugin` with description, category, `Win64` in `PlatformAllowList`, `Resources/Icon128.png`, a `README.md`, a `CHANGELOG.md`, a licence file, no hardcoded paths, no third-party binaries we don't have rights to redistribute. It should pass `RunUAT BuildPlugin` cleanly for each target engine version once it reaches the PC.
4. Idempotent and non-destructive: the wizard can be re-run. It never silently overwrites existing project settings or assets — it shows a diff/summary of what it will change and asks for confirmation. It writes config through the proper `UDeveloperSettings` / `UObject::SaveConfig` paths rather than hand-editing `.ini` files where possible.
5. Localisation-ready: every UI string goes through `LOCTEXT`. Ship English (`en`) and Welsh (`cy`) localisation. Welsh is not optional — it's part of the brand. Put all translatable strings in one place so I can review the Welsh.
6. No engine-level plugin install required: everything is a project plugin. Third-party plugins we rely on (Mo-Sys Live Link, Blackmagic, AJA) are detected, not bundled.

## What it does

Add a toolbar button and a `Tools → vedrí VP Toolkit` menu entry that opens a Setup Wizard (Slate, modal window, multi-step with back/next/apply). First screen has two radio buttons:

* Set up Green Screen shoot
* Set up LED shoot

Each subsequent step is a group of checkboxes with sensible defaults for the chosen mode. Everything below is a step unless marked otherwise.

### Step 1 — Enable required plugins

Enable (and prompt for the editor restart that needs) the engine plugins the chosen mode requires. Verify the exact plugin names against the engine; expected set:

* Both modes: Media Framework Utilities, Media IO Framework, Live Link, Live Link FreeD, Live Link Lens, Camera Calibration, Lens Distortion, Timed Data Monitor, Remote Control API, Virtual Production Utilities, Takes Recorder, OpenColorIO, Switchboard
* Video I/O: Blackmagic Media Player and/or AJA Media Player depending on the card chosen in Step 2
* LED mode only: nDisplay, nDisplay Support, ICVFX / In-Camera VFX, Live Link Over nDisplay, Multi-User Editing, Virtual Production utilities for light cards
* Green screen mode only: nothing extra (we do not key in Unreal — see Step 5)
* Detect and enable Mo-Sys Live Link if the plugin is present in the project or engine; if not, show a notice with the download link and fall back to FreeD.

### Step 2 — Video I/O card

Radio: Blackmagic DeckLink / AJA. Try to auto-detect which SDK plugin is enabled and which devices are present, pre-select, allow override. Everything downstream (timecode provider, custom time step, media sources, media outputs) is built for the chosen card family. Design the code so adding a third card family later is one new class, not a rewrite.

### Step 3 — Timing: frame rate, genlock, timecode

* Frame rate presets with UK broadcast defaults: default 25 fps, with 50, 24, 23.976, 29.97, 30, 59.94, 60 selectable. Resolution presets: 1920×1080 default, 3840×2160 selectable, plus custom.
* Set the project's fixed frame rate, disable smooth frame rate, set the engine's Custom Time Step to the card's genlock time step class (Blackmagic or AJA), and set the Timecode Provider to the card's timecode provider class. Let the user pick the device/port and whether genlock is from the SDI input or reference input.
* Create a Media Profile asset (in `/Game/VPToolkit/Config/`) that captures: timecode provider, custom time step, media source proxies, media output proxies. Set it as the project's active Media Profile. Explain in a comment why we use Media Profiles rather than raw engine settings.
* Add a Timing health panel (non-modal dockable tab, also reachable from the menu) that shows live: genlock state (locked / not locked / no signal), timecode provider state and current timecode, engine frame rate vs. target. Use the Timed Data Monitor where it makes sense instead of reinventing it.

### Step 4 — Video Out

* Create a Media Output asset for the chosen card (SDI output, format matching Step 3, 10-bit YUV default with RGB/8-bit as options, key+fill option where the card supports it).
* Create a Media Capture setup that outputs the active viewport (or a chosen Cine Camera) to that SDI output, with a one-click Start / Stop Output button in the health panel and on the toolbar.
* Set the output to respect the genlock so frames leave on the clock.

### Step 5 — Mode-specific setup

#### Green screen mode

* Create a Media Source for the camera plate (card input matching Step 3) and a Media Plate / Media Bundle so the live plate can be viewed in-editor for line-up.
* Do not set up Composure or any chroma keying. Our workflow is: camera plate goes to an external keyer/switcher; Unreal sends clean CG out over SDI (Step 4). Document this clearly in the generated README for the project.
* Optionally (checkbox) create a second Media Output for a matte/alpha or key feed if the card supports it, so the external keyer has what it needs.
* Create a `BP_VP_GreenScreen_Stage` level with: the tracked Cine Camera (Step 6), a floor-plane reference at real-world scale, and a placeholder background. Spawn nothing heavy.

#### LED mode

* Show a Wall specs form: wall width (m), wall height (m), pixel pitch (mm), panel/tile size (mm), number of render nodes, optional ceiling (w × d, pitch). Defaults pre-filled to our planned wall: 8 m × 3 m. Derive wall resolution from dimensions and pitch and show it live.
* From the form, generate an nDisplay Config asset (`/Game/VPToolkit/nDisplay/`): a cluster with the specified node count, viewports split evenly across nodes, a mesh/screen matching the wall geometry (flat by default; curvature radius optional), and an ICVFX camera component with sensible inner-frustum defaults (resolution, overscan, soft edge). Also add default light cards and chromakey markers settings so people can tweak rather than build from scratch.
* Generate a Switchboard config stub pointing at the nDisplay config, with placeholder node names/IPs and a note on how to fill them in.
* Create a `BP_VP_LED_Stage` level containing the nDisplay root actor, the tracked Cine Camera, and a simple gray-box set at real scale.
* Apply the known nDisplay-friendly project settings (verify each against the engine): disable VSync, appropriate texture streaming settings, multi-GPU opt-in if more than one GPU is detected, and anything else the ICVFX quick-start docs recommend.

### Step 6 — Camera tracking and lens

* Tracking source radio: Mo-Sys StarTracker (via Mo-Sys Live Link plugin if present, otherwise FreeD) / Generic FreeD (UDP port, subject name).
* Create a Live Link preset asset with the chosen source and subject pre-configured, and set it to auto-apply on editor start.
* Create a `BP_VP_TrackedCineCamera` actor: Cine Camera + Live Link Controller driving transform (and FIZ where the source supplies it) + Lens component referencing a Lens File asset. Create an empty Lens File with a note that calibration is done through the Camera Calibration plugin's tools; don't try to calibrate.
* Sensor defaults: let the user pick from a short list of common cameras (include Blackmagic Pyxis 6K / URSA sensor sizes) or enter custom sensor dimensions.

### Step 7 — Project settings hardening (both modes)

A reviewed checklist of settings the wizard will apply, each with a checkbox and a one-line reason shown in the UI. Include, after verifying against the engine: fixed frame rate, smooth frame rate off, motion blur off by default, temporal upsampling considerations, "use less CPU when in background" off, default map set to the generated stage level, OpenColorIO config placeholder, Remote Control preset stub, Takes Recorder defaults (timecode-stamped take names).

### Presets

* Save / Load wizard presets as JSON (`Saved/VPToolkit/Presets/*.json`) so a studio can keep "Our 8m wall, 2 nodes, Blackmagic, 25p" and reapply it to a new project in one click. Ship two bundled presets: `vedri-greenscreen-25p-blackmagic` and `vedri-led-8x3m-25p-blackmagic`.

### Summary / Apply

Final step shows everything that will be created or changed, grouped by plugins / settings / assets, with a per-item tick. On Apply: do the work, log each action to the Output Log under a `LogVPToolkit` category, write a `VPToolkit_SetupReport.md` into the project's `Saved/VPToolkit/` folder, and offer to restart the editor if plugin enablement requires it.

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
    VedriVPToolkitEditor/     # Wizard UI, menu/toolbar, setup actions
      Private/Wizard/         # Slate steps, one class per step
      Private/Actions/        # One class per "setup action" (EnablePlugins, ConfigureTiming, CreateMediaProfile, GenerateNDisplayConfig, ...) implementing a common interface: Preview() -> list of changes, Apply(), Validate()
      Private/IO/             # Card family abstraction: IVPVideoIOProvider with Blackmagic and AJA implementations
      Private/Tracking/       # Tracking source abstraction: Mo-Sys, FreeD
      Private/Health/         # Dockable timing/health tab
  Docs/
    README.md, CHANGELOG.md, LICENSE, USER_GUIDE.md (with screenshots placeholders)
```

Key design rule: every setup action is a small class with Preview / Apply / Validate. The wizard just composes them. That's what makes it idempotent, testable, and easy to extend.

## Quality bar

* Written to compile with zero warnings on all target engine versions on Win64. Since you can't compile here, the review pass in the working rules is the substitute, and `docs/WINDOWS_TODO.md` is the honest list of what's unproven.
* Standalone `clang++` tests on the Mac for the engine-independent core (preset round-trip, nDisplay resolution/viewport maths from wall specs, diff model). These must pass before each milestone is called done.
* Unreal Automation tests (run on the PC later) covering the same core plus: action Preview returning correct diffs against a known project state, version guards, and card-family fallbacks reporting correctly.
* A `BuildPlugin.ps1` that runs `RunUAT BuildPlugin` for each target engine version and reports pass/fail, plus a `RunTests.ps1` that runs the Automation tests headlessly (`UnrealEditor-Cmd ... -nullrhi`) — both ready for day one on the PC.
* A `CLAUDE.md` in the repo root capturing the conventions you settle on (naming, where actions live, how to add a card family or tracking source), so future sessions stay consistent.
* Commit in small, buildable steps with clear messages.

## How I want to work

1. Plan first. Get the reference source (rule 1 above), read the Blackmagic, AJA, nDisplay, Live Link and MediaFrameworkUtilities modules, and tell me which class names / settings you'll actually use, with any 5.5 → latest differences you found. If the clone doesn't work, stop and tell me before writing code.
2. Ask me before: choosing between two reasonable architectures, anything that touches licensing/redistribution, and anything in the nDisplay generation you're not confident about.
3. Build in milestones. Each one ends with the review pass, passing standalone tests for anything in the core layer, and an updated `WINDOWS_TODO.md`:
   * M1: plugin skeleton, `.uplugin`, `.Build.cs` files, menu/toolbar registration, wizard shell with mode selection, plugin-enablement action. Core layer + standalone test harness set up.
   * M2: presets (schema, save/load, two bundled presets), project-settings hardening actions, Summary/Apply flow with diff preview and setup report.
   * M3: tracking + lens + tracked camera (Live Link preset, FreeD, Mo-Sys detection, Lens File, `BP_VP_TrackedCineCamera`).
   * M4: timing + Video Out + green-screen mode — card-family abstraction, Blackmagic and AJA implementations written against the headers, Media Profile creation, health panel.
   * M5: LED mode — wall-specs form, resolution/viewport maths in core with tests, nDisplay config generation, Switchboard stub, LED stage level.
   * M6: localisation (en + cy), docs, Fab packaging, Automation tests, `BuildPlugin.ps1`, `RunTests.ps1`, final `WINDOWS_TODO.md`.
   * M7 (when the PC arrives): first Win64 compile, work through `WINDOWS_TODO.md`, test against real DeckLink/AJA + StarTracker, nDisplay on real nodes.
4. After each milestone, write the manual test steps for the PC into `docs/TEST_PLAN.md` — which menu, what to click, what should happen, what to check on the SDI monitor / StarTracker — written so I can hand them to a freelancer on the day the PC is ready.
