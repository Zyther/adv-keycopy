# Cardputer-Adv Key Copier

Port of [zinongli/KeyCopier](https://github.com/zinongli/KeyCopier) (Flipper Zero v1.5) to the [M5Stack Cardputer-Adv](https://docs.m5stack.com/en/core/Cardputer-Adv). Measure a physical key by laying it on the screen, matching a 1:1 contour, and recording bitting. Save and load Flipper-compatible `.keycopy` files on microSD.

## Goal

A full working Adv firmware with the same job as Flipper Key Copier 1.5: all 23 key formats (including double-sided), measure by overlay, MACS-checked depths, save/load, and help. Adapted for a 240×135 / 1.14" panel and a 56-key keyboard.

Out of scope: Wi-Fi, a companion phone app, the Flipper QR/video screen, UiFlow, and key cutting / 3D-print export.

## Hardware and stack

| Item | Value |
| --- | --- |
| Device | M5Stack Cardputer-Adv (K132-Adv), Stamp-S3A, ESP32-S3FN8 |
| Display | ST7789V2, 240×135, 1.14" IPS |
| Keyboard | 56-key (4×14), TCA8418 |
| Storage | microSD (SPI) |
| Build | PlatformIO + Arduino |
| HAL | M5Cardputer (pulls in M5Unified). Official Adv `platformio.ini` flags from M5 docs. |

Flipper comparison used for scale:

- Flipper: 128×64 on 1.4", `INCHES_PER_PX = 0.00978` → ~1.25" × 0.63" active area.
- Adv: 240×135 on 1.14" (16:9) → ~0.994" × 0.559" active area, default `0.004140` in/px.

The Adv panel is physically smaller. Long formats (SC4, Y2, B102, and others) and some double-sided blades do not fit on the glass at once. The contour stays 1:1; the view pans.

## Decisions (locked)

- True 1:1 overlay with pan when the key is larger than the panel. No scale-to-fit.
- Measure input is type-first: digit sets the selected pin; arrows still change pin and nudge depth.
- Files are Flipper-compatible `.keycopy` under `/adv-keycopy` or `/sd/adv-keycopy` (folder `adv-keycopy` at the card root).
- Physical pitch is hardcoded to the Adv default, with a Settings field to tweak and persist it.
- QR/video instruction screen is dropped.

## Architecture

Single firmware process. No network.

```
app_shell ──► measure_view ──► key_geometry ──► key_formats
    │              │
    │              └── model (format, depths, pin, pan, pitch)
    │
    ├── keycopy_io ──► /adv-keycopy/*.keycopy (or /sd/adv-keycopy)
    └── settings ────► SD file, or NVS if SD is missing
```

Screens are a state machine owned by `app_shell`: Main menu, Manufacturer list, Format list, Measure, Save, Load, Settings, Help.

The Flipper draw loop is split: `key_geometry` emits inch-space segments; `measure_view` converts to pixels, applies pan, and draws. Geometry is host-testable without the Adv.

## Components

### `key_formats`

Read-only table of the same 23 `KeyFormat` records as Flipper 1.5. Fields match the upstream struct: manufacturer, format name, data-sheet link, sides, stop, pin spacing (first/last/increment), pin count and width, drill angle, elbow, uncut/deepest depth, depth step, min/max depth index, MACS, clearance.

Unset Flipper `sides` / `stop` mean single-sided / shoulder-stopped. In this port, singles are stored as `sides = 1` and `stop = 1` so the table is explicit. Behavior stays `sides == 2` and `stop == 2` for the dual-sided and tip-stopped cases.

### `key_geometry`

Input: `KeyFormat` + depth array (`pin_num` entries, plus the Flipper-style padding slot used for neighbor lookups). Output: a list of line segments in inches (x along the blade from the shoulder, y down from the uncut top).

Includes: top shoulder, pin flats, drill-angle slopes, intersection vs clearance (MACS-adjacent cuts that meet), elbow/tip, optional stop line, and the mirrored bottom edge when `sides == 2`. Double-sided keys use the same depth values on both edges, as Flipper does.

Also exports `depth_change_allowed(format, depths, pin_index, new_depth)` so MACS and min/max checks are host-testable. `measure_view` calls this before writing a pin. No canvas types. No pixel math.

### `measure_view`

Converts inch segments to pixels:

```
px = inch / inches_per_px - pan
```

Default `inches_per_px = 0.004140`. Settings may replace it. Pan is stored and applied in pixels (`pan_x`, `pan_y`).

Drawing rules:

- Contour coordinates are locked to physical glass. Labels and HUD must not shift the contour.
- Pin depth digits sit near each pin in leftover pixels above the blade when there is room; otherwise they sit in a thin status strip that does not consume overlay space (drawn over unused margin, not by shrinking the key).
- Selected pin gets a marker.
- Format name and full bitting string live in unused margin (typically top-right or a 10-px status line in pixels that are not part of the 1:1 blade).

Input (measure screen):

| Key | Action |
| --- | --- |
| `←` or `,` | Previous pin |
| `→` or `.` | Next pin |
| `↑` or `;` | Shallower (decrease depth index) |
| `↓` or `/` | Deeper (increase depth index) |
| `0`–`9` | Set selected pin to that depth if in range and MACS allows |
| `-` | Pan left |
| `=` | Pan right |
| `[` | Pan up |
| `]` | Pan down |
| `ESC` or `` ` `` | Back to menu |

When the selected pin’s 1:1 position is off-panel, auto-pan just enough to bring that pin on-screen. Manual pan is clamped to contour bounds so the key cannot be lost in empty space.

MACS and min/max depth: illegal digit or arrow is ignored. No clamp-to-illegal, no dialog.

### `keycopy_io`

Directory: `/adv-keycopy` (fallback `/sd/adv-keycopy`). Create it on first successful SD mount if missing. The folder on the FAT volume is `adv-keycopy` at the card root.

File shape matches Flipper Format v1 (same keys, same header string):

```
Filetype: Flipper Key Copier File
Version: 1
Manufacturer: Kwikset
Format Name: KW1
Data Sheet: https://lsamichigan.org/Tech/Kwikset_KeySpecs.pdf
Number of Pins: 5
Maximum Adjacent Cut Specification (MACS): 4
Bitting Pattern: 1-2-3-4-5
```

Load matches `Format Name` to `key_formats`. Bitting must be `pin_num` single digits separated by `-` (example: five pins → `1-2-3-4-5`, string length `2 * pin_num - 1`). Parse as Flipper does: the character at index `i * 2` is pin `i` (`0`–`9`). A couple of Flipper formats list `max_depth_ind = 10`; save still uses `%d`, and load still reads one character per slot — same limitation as Flipper.

Unknown format, missing required fields, or a bitting string that is not exactly `pin_num` hyphen-separated digits: reject the whole file. Do not half-apply.

### `app_shell`

Owns `KeyCopierModel`: current format, `depth[]`, selected pin, pan, pitch, key name, `data_loaded`.

Boot: init M5, try SD mount, load pitch from `settings.ini` in `/adv-keycopy` or `/sd/adv-keycopy` if present else NVS else default, start at main menu. Initial format is KW1, all pins at `min_depth_ind`, pan at origin (shoulder at the left of the glass).

Menu items: Select Key Format, Measure, Save, Load, Settings, Help.

Format pick: unique manufacturers → formats for that manufacturer → copy into model, reset depths, pin 1, pan origin, then go to Measure.

Save: keyboard name prompt → write `<name>.keycopy` into `/adv-keycopy` (or `/sd/adv-keycopy` if that is the mounted path). Reject empty names and names that contain `/ \ : * ? " < > |` or are `.` / `..`. Stay on the prompt with a short error. Success returns to the menu. The folder name on the FAT volume is `adv-keycopy` at the card root (`/sd` is only a mount prefix when present).

Load: list `*.keycopy` in that folder (not the SD root). Pick one to apply.

Settings: edit inches-per-pixel. Persist to `settings.ini` in `/adv-keycopy` or `/sd/adv-keycopy` when SD is mounted; otherwise NVS. File contents are one line: `inches_per_px=0.004140`. Reject non-positive values; junk/missing falls back to `0.004140`.

Help: short scroll/text — place key on glass, align shoulder, set each pin, look with one eye closed. Point at the GitHub original and this repo. No QR widget.

## Data flow

1. Boot → menu.
2. Select format → model reset → Measure.
3. Each Measure frame: geometry(format, depths) → inch segments → scale + pan → draw. Input updates depths, pin, or pan.
4. Save → name → Flipper-format file on SD.
5. Load → parse → full model replace or no-op on error.
6. Settings → pitch in memory + persist.

No-SD: Measure and menu still work. Save and Load show `NO SD` and return. Pitch can still persist in NVS. Never write a `.keycopy` into flash.

## Error handling

| Case | Behavior |
| --- | --- |
| SD missing or mount fail | `NO SD` on Save/Load; rest of app works |
| Save write fail / bad name | Stay on name prompt; show reason |
| Load parse/format/bitting fail | Keep current model; show reason |
| MACS or depth out of range | Ignore the key |
| Pan past contour | Clamp |
| Invalid stored pitch | Use `0.004140` |

## Testing

### Host (no hardware)

Native PlatformIO env (or equivalent) links `key_geometry` and `keycopy_io` only.

- KW1 and SC4: pin-center inches, shoulder, elbow match the format table.
- Ford H75: mirrored bottom edge present.
- MACS: legal neighbor change accepted; illegal rejected.
- `.keycopy` round-trip: header, field names, `d-d-d` bitting.
- Reject unknown `Format Name` and wrong-length bitting.

### On-device smoke

Flash Adv → KW1 with a real key on the glass → digit then arrow on one pin → pan SC4 or B102 so the selected pin stays 1:1 on screen → save under `/adv-keycopy` or `/sd/adv-keycopy` → reload → tweak pitch in Settings and confirm the overlay moves → unmount/no-SD Save/Load shows `NO SD`.

## Success criteria

- All 23 Flipper 1.5 formats selectable.
- Overlay is physically 1:1; overflow is pan, not scale.
- Type-to-set depth with arrow fallback; MACS enforced.
- Flipper-readable `.keycopy` files in `/adv-keycopy` or `/sd/adv-keycopy`.
- Pitch default plus persisted Settings tweak.
- Host tests above pass; on-device smoke passes.

## Source layout (implementation)

```
platformio.ini
src/main.cpp
src/app_shell.*
src/measure_view.*
src/key_geometry.*
src/key_formats.*
src/keycopy_io.*
test/           # native host tests
README.md
docs/superpowers/specs/2026-08-30-adv-key-copier-design.md
```

Client-facing usage lives in `README.md` (flash, measure, keys, formats, files, settings, limits). This spec is for implementers.
