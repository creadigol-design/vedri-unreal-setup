# Validation codes

The core layer never produces user-facing text. Each finding carries a stable **code** plus positional
**arguments**, and the editor turns the code into a localised message with `FText::Format` (arguments
fill `{0}`, `{1}`, …). This file is the catalogue of every code. It's where the English wording gets
agreed and the Welsh gets reviewed before both go into the editor's `LOCTEXT` table and the `cy` `.po` file.

`Tests/Standalone/check_validation_codes.py` runs as part of `make test`. It fails if Core emits a code
that's missing here, or if this file lists a code Core no longer emits.

Severity: **E** error (blocks Apply), **W** warning (shown, does not block), **I** info.

The English column is a draft. The Welsh (`cy`) column gets filled in M6 and reviewed by a fluent speaker.

## Presets

| Code | Sev | Arguments | English (draft) | Cymraeg |
|---|---|---|---|---|
| `preset.invalidJson` | E | {0} parser message | This preset file isn't valid JSON ({0}). | |
| `preset.notAnObject` | E | – | This preset file doesn't contain a JSON object. | |
| `preset.missingSchemaVersion` | E | – | This file has no valid schemaVersion, so it isn't a VP Toolkit preset. | |
| `preset.newerSchemaVersion` | E | {0} file version, {1} supported version | This preset was saved by a newer version of the toolkit (schema {0}; this version reads up to {1}). Update the plugin to load it. | |
| `preset.wrongType` | E | {0} expected type id: `boolean`, `number`, `unsignedInteger`, `integer`, `string`, `object` | This setting has the wrong type: expected {0}. | |
| `preset.unknownEnumValue` | E | {0} value found | "{0}" isn't a valid choice for this setting. | |
| `preset.unknownField` | W | {0} field name | Unknown setting "{0}" was ignored. Check it for a typo. | |
| `preset.unknownSelectionKey` | W | {0} key | Ignored a saved Summary tick for "{0}": it isn't a recognised item. | |

## Video I/O

| Code | Sev | Arguments | English (draft) | Cymraeg |
|---|---|---|---|---|
| `videoIO.unknownCardFamily` | E | {0} family id | No video I/O support is available for "{0}". Enable its plugin or choose another card. | |
| `videoIO.deviceIndexOutOfRange` | E | {0} index | Device {0} is out of range (0–15). | |
| `videoIO.portOutOfRange` | E | {0} connector | Connector {0} doesn't exist (1–16). | |
| `videoIO.inputOutputSamePort` | E | {0} connector | The camera input and the video output can't share connector {0}. | |
| `greenScreen.mattePortInUse` | E | {0} connector | Matte output connector {0} is already used by the camera input or the main output. | |

## Timing

| Code | Sev | Arguments | English (draft) | Cymraeg |
|---|---|---|---|---|
| `timing.invalidFrameRate` | E | {0} numerator, {1} denominator | Frame rate {0}/{1} isn't valid. | |
| `timing.nonStandardFrameRate` | W | {0} rate label | {0} fps isn't a standard broadcast rate. Check your camera and card support it. | |
| `timing.dropFrameNotSupported` | E | {0} rate label | Drop-frame timecode only exists at 29.97 and 59.94 fps, not {0}. | |
| `timing.resolutionOutOfRange` | E | {0} width, {1} height | Resolution {0} × {1} is outside 16–8192 pixels. | |
| `timing.resolutionWidthOdd` | E | {0} width | The width ({0}) must be even for 4:2:2 video. | |
| `timing.customResolution` | I | {0} width, {1} height | Custom resolution {0} × {1}: check that your card supports it. | |
| `timing.sdiLinkRequired` | I / W | {0} link id: `1.5G`, `3G`, `6G`, `12G`, `beyond12G`; {1} width, {2} height, {3} rate label | I: {1} × {2} at {3} fps needs {0}-SDI. W (`beyond12G`): {1} × {2} at {3} fps is beyond single-link 12G-SDI. | |

## LED wall

Fields are relative to `led.wall` (e.g. `led.wall.widthM`, `led.wall.ceiling.depthM`).

| Code | Sev | Arguments | English (draft) | Cymraeg |
|---|---|---|---|---|
| `wall.dimensionOutOfRange` | E | {0} metres | Dimensions must be more than 0 and at most 100 m (got {0} m). | |
| `wall.pitchOutOfRange` | E | {0} mm | Pixel pitch must be more than 0 and at most 50 mm (got {0} mm). | |
| `wall.tileSizeOutOfRange` | E | {0} mm | Tile size must be more than 0 and at most 2000 mm (got {0} mm). | |
| `wall.pixelsPerTileOutOfRange` | E | optional {0} derived value | Pixels per tile must be between 1 and 8192. | |
| `wall.smallerThanOneTile` | E | {0} metres, {1} tile mm | {0} m is smaller than one {1} mm tile. | |
| `wall.sizeNotWholeTiles` | W | {0} entered metres, {1} tiles, {2} actual metres | {0} m isn't a whole number of tiles: using {1} tiles ({2} m). | |
| `wall.pitchNotWholePixelsPerTile` | W | {0} tile mm, {1} pitch mm, {2} exact pixels, {3} pixels used | A {0} mm tile at {1} mm pitch is {2} px, so the exact count is uncertain: using {3}. Enter pixels per tile from the panel spec sheet. | |
| `wall.nodeCountOutOfRange` | E | {0} count | Use between 1 and 64 render nodes (got {0}). | |
| `wall.moreNodesThanTileColumns` | E | {0} nodes, {1} tile columns | {0} render nodes but only {1} tile columns: each node needs at least one column. | |
| `wall.unevenNodeSplit` | I | {0} widest px, {1} narrowest px | The wall doesn't split evenly: nodes render {0} or {1} px wide. | |
| `wall.nodeWindowTooLarge` | W | {0} node id, {1} width, {2} height | {0} would render {1} × {2} px, more than a GPU handles reliably. Add render nodes. | |
| `wall.curvatureNegative` | E | {0} metres | Curvature radius can't be negative (got {0} m). | |
| `wall.curvatureTooTight` | E | {0} radius m, {1} wall width m | A {1} m wall can't curve on a {0} m radius: it would wrap more than a full circle. | |
| `wall.bottomHeightOutOfRange` | E | {0} metres | The wall's bottom edge must be 0–20 m above the floor (got {0} m). | |
| `wall.ceilingHeightOutOfRange` | E | {0} metres | Ceiling height must be 0–30 m (got {0} m). Use 0 for level with the top of the wall. | |
| `wall.ceilingNodeOutOfRange` | E | {0} node index, {1} node count | Ceiling render node {0} doesn't exist: the cluster has {1} nodes. | |

## Tracking and lens

| Code | Sev | Arguments | English (draft) | Cymraeg |
|---|---|---|---|---|
| `tracking.unknownSource` | E | {0} source id | Unknown tracking source "{0}". | |
| `tracking.udpPortOutOfRange` | E | {0} port | The UDP port must be 1–65535 (got {0}). | |
| `tracking.subjectNameEmpty` | E | – | Enter a Live Link subject name. | |
| `lens.unknownCameraPreset` | W | {0} preset id | Unknown camera "{0}": the sensor size is used as entered. | |
| `lens.sensorDiffersFromPreset` | W | {0} camera name, {1} width mm, {2} height mm | {1} × {2} mm doesn't match the {0}'s sensor. Choose Custom if you're shooting in a crop mode. | |
| `lens.sensorSizeOutOfRange` | E | {0} mm | Sensor dimensions must be more than 0 and at most 100 mm (got {0} mm). | |

## Summary / Apply

| Code | Sev | Arguments | English (draft) | Cymraeg |
|---|---|---|---|---|
| `changes.conflict` | E | {0} first step, {1} second step, {2} first value, {3} second value | Two steps want different values for this item ({0}: {2}, {1}: {3}). This is a toolkit bug; please report it. | |
