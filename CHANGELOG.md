# Changelog

All notable changes are recorded here. Versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Engine-independent core (`VedriVPToolkitEditor/Private/Core/`):
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
- Bundled presets `vedri-greenscreen-25p-blackmagic` and `vedri-led-8x3m-25p-blackmagic`.
- Standalone test suite (56 tests) built with clang or gcc, without Unreal.
- Project docs: brief, plan and decisions, Windows TODO, test plan, validation-code catalogue.
