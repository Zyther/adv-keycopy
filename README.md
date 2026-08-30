# Key Copier for Cardputer-Adv

Measure a house or vehicle key by laying it on the Cardputer-Adv screen and matching a life-size outline. The app is a port of [Key Copier for Flipper Zero](https://github.com/zinongli/KeyCopier) (v1.5): same formats, same bitting files, same “one eye closed” overlay method — rebuilt for the Adv’s smaller color screen and 56-key keyboard.

Use it on keys you own or are asked to duplicate lawfully. It does not cut keys and it does not open locks.

## What you need

- [M5Stack Cardputer-Adv](https://docs.m5stack.com/en/core/Cardputer-Adv) (K132-Adv)
- USB-C cable
- A microSD card if you want to save or load measurements (optional for measuring only)
- [PlatformIO](https://platformio.org/) (CLI or the VS Code / Cursor extension) to build and flash

The original Cardputer (non-Adv) is not the target. Keyboard and audio chips differ.

## The small-screen catch

Flipper’s panel is about 1.25" wide. The Adv is about 1.00" wide. The outline stays **true 1:1** so a real key still lines up on the glass. Keys that are longer than the display (Schlage SC4, many 6-pin house keys, several car keys) will not fit on screen at once. **Pan** the drawing and match the key a section at a time. Do not scale the outline down — that would break the measurement.

## Flash the firmware

1. Clone this repo and open it in PlatformIO.
2. Put the Adv in download mode: set the side power switch to **OFF**, hold **G0** on the Stamp, switch power **ON**, then release G0.
3. Upload:

```bash
pio run -t upload
```

4. Power-cycle if the serial port does not come back. Charging only works with the side switch **ON**.

## First-time setup

1. For save/load, format a microSD as FAT32, insert it, and power on. The app creates a folder named `adv-keycopy` at the card root the first time it mounts successfully. On a computer that folder is `adv-keycopy/`. On the Adv the same folder is `/adv-keycopy` or `/sd/adv-keycopy`.
2. Open **Measure** (or pick a format first). Leave **Settings** at the default pitch unless the outline is clearly the wrong size for a known key.

## Measure a key

1. **Select Key Format** — pick the manufacturer, then the keyway (KW1, SC4, H75, …). If you are unsure, the stamps on the key head or a locksmith key-blank list will tell you.
2. **Measure** — you land on a life-size contour for that blank.
3. Lay the key **on the screen**. Align the shoulder (or the tip, on tip-stopped car keys) with the drawn shoulder/stop.
4. Close one eye so you are not looking around the glass.
5. For each pin:
   - `,` or **Left** / `.` or **Right** — select that pin.
   - Type **0–9** to set the depth, or `;` / **Up** and `/` / **Down** to nudge.
   - If that pin is off the screen, the view jumps to it. You can also pan with `-` `=` `[` `]`.
6. When every cut matches the outline, **Save** if you have an SD card.

Adjacent cuts cannot differ by more than that format’s **MACS** (maximum adjacent cut specification). An illegal depth is ignored; the previous value stays.

Double-sided keys (Ford H75, Chevy B102, several motorcycle and RV blanks) draw both edges. The same numbers apply to both sides.

## Keyboard

### Everywhere

| Key | Action |
| --- | --- |
| `;` / `,` / `.` / `/` or arrows | Move in lists and on the measure screen |
| **Enter** | Select |
| **ESC** or `` ` `` | Back |

### Measure screen

| Key | Action |
| --- | --- |
| `,` or Left | Previous pin |
| `.` or Right | Next pin |
| `;` or Up | Shallower cut |
| `/` or Down | Deeper cut |
| `0`–`9` | Set the selected pin to that depth (if legal) |
| `-` / `=` | Pan left / right |
| `[` / `]` | Pan up / down |
| **ESC** or `` ` `` | Menu |

### Save

Type a name with the keyboard and press **Enter**. Files are stored as `adv-keycopy/<name>.keycopy` on the SD card (`/adv-keycopy` or `/sd/adv-keycopy` on the device).

## Save and load (Flipper-compatible)

Files are the same **Flipper Key Copier** format the Flipper app writes. You can copy a `.keycopy` between a Flipper and the Adv.

Example:

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

- **Location:** folder `adv-keycopy/` at the SD card root (device path `/adv-keycopy` or `/sd/adv-keycopy`). Not loose files on the card root.
- **Load** only lists `*.keycopy` in that folder. A file that is not a Key Copier file, names an unknown format, or has the wrong number of depths is rejected; your current measurement is left alone.
- **No SD card:** you can still measure. Save and Load show `NO SD` and do nothing else.

## Settings (scale)

**Settings** has one user control: **inches per pixel**.

The Adv ships with `0.004140` (from the 1.14" / 240×135 panel). If a known blank does not line up — the shoulder fits but the last pin is short or long — change this number slightly and save. The value is stored in `settings.ini` in that same folder (`/adv-keycopy` or `/sd/adv-keycopy`) when an SD card is present, otherwise in on-chip NVS.

Do not use Settings to “zoom to fit.” That is the wrong tool; pan instead.

## Supported formats

Same set as Key Copier 1.5.

| Manufacturer | Format | Pins | Notes |
| --- | --- | --- | --- |
| Kwikset | KW1 | 5 | Common residential |
| Schlage | SC4 | 6 | Long; you will pan |
| Arrow | AR4 | 6 | |
| Master Lock | M1 | 5 | |
| American | AM7 | 6 | |
| Yale | Y2 | 6 | Long; you will pan |
| Yale | Y11 | 5 | |
| Sargent | S22 | 6 | |
| National | NA25 | 5 | |
| Corbin | CO88 | 6 | Long; you will pan |
| Lockwood | LW4 | 5 | |
| Lockwood | LW5 | 6 | Long; you will pan |
| National | NA12 | 5 | |
| Russwin | RU45 | 6 | |
| Weiser | WR3 | 5 | |
| Ford | H75 | 8 | Double-sided, tip-stopped |
| Chevrolet | B102 | 10 | Double-sided, tip-stopped; pan |
| Dodge | Y159 | 8 | Double-sided, tip-stopped |
| Kawasaki | KA14 | 6 | Double-sided |
| Suzuki | SUZ18 | 7 | Double-sided |
| Yamaha | YM63 | 7 | Double-sided |
| Best (A2) | SFIC | 6 | Tip-stopped |
| RV (FIC, GL, Bauer) | RV | 5 | Double-sided |

Format geometry (pin spacing, depth steps, MACS, drill angle) is copied from the Flipper tables, which are in inches.

## Build from source

```bash
pio run              # compile
pio run -t upload    # flash (Adv in download mode)
pio test             # host tests for geometry and .keycopy files
```

See `docs/superpowers/specs/2026-08-30-adv-key-copier-design.md` for the implementer spec.

## Limits

- The outline is only as accurate as the panel pitch. Tweak Settings if your unit’s glass does not match the default.
- You still need the correct **blank / keyway**. This app measures cut depths; it does not identify an unknown keyway from a photo.
- Bitting in the file is single-digit `0`–`9` per pin, same as Flipper.
- Not a cutting machine. Take the bitting to a locksmith, a punch/code machine, or a 3D-print workflow you already trust.

## Credits

- Original Flipper app: [zinongli/KeyCopier](https://github.com/zinongli/KeyCopier) by Torron, MIT License, with formats and double-sided support from HonestLocksmith, Offreds, lightos, and RIcePatrol.
- Hardware docs: [M5Stack Cardputer-Adv](https://docs.m5stack.com/en/core/Cardputer-Adv).
