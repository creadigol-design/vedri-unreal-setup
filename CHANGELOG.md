# Changelog

All notable changes are recorded here. Versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Engine-independent core (`VedriVPToolkitEditor/Public/Core/`):
  - LED wall maths: whole-tile grid and resolution from size and pitch (handles rounded published
    pitches), tile-aligned split across render nodes, optional ceiling, flat or curved surface geometry
    and viewport meshes.
  - Frame-rate and resolution presets (UK default 25p, 1920 × 1080), drop-frame rules and an SDI
    link-class hint.
  - Wizard configuration model, validation by stable code, and camera sensor presets.
  - Preset format, schema version 1: JSON reader/writer, strict types, warnings for unknown fields, a
    file-name slug that folds Welsh accents.
  - Summary/Apply diff model (create / modify / regenerate / overwrite / unchanged / unavailable,
    non-destructive defaults, conflict detection, per-item ticks saved in presets) and the setup report.
  - Provider-based tracking config: a provider id plus provider-owned settings, eleven known
    providers, FreeD by default, no invented port defaults.
- Bundled presets `vedri-greenscreen-25p-blackmagic-mosys`, `vedri-led-8x3m-25p-blackmagic-mosys`,
  `greenscreen-freed` and `led-freed`.
- Standalone test suite (56 tests) built with clang or gcc, without Unreal.
- Project docs: brief (revised for UE 5.5–5.8 on the Windows PC), plan and decisions, Windows TODO,
  test plan, validation-code catalogue, tracking provider research notes.
