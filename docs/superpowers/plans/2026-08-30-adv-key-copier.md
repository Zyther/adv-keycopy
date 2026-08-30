# Cardputer-Adv Key Copier Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship Flipper Key Copier 1.5 as Cardputer-Adv firmware: 1:1 overlay with pan, type-to-set depths, all 23 formats, Flipper `.keycopy` on SD.

**Architecture:** Host-testable core (`key_formats`, `key_geometry`, `keycopy_io`, `view_math`) with no Arduino types. Device shell (`measure_view`, `app_shell`, `main`) uses M5Cardputer for display, TCA8418 keyboard, and SD. Inch-space contours are converted to pixels with `0.004140` in/px and a pixel pan offset.

**Tech Stack:** PlatformIO, Arduino, espressif32@6.7.0, M5Cardputer (M5Unified), Unity native tests.

## Global Constraints

- Device: M5Stack Cardputer-Adv (K132-Adv), ESP32-S3FN8, ST7789V2 240×135, TCA8418 keyboard.
- Build: PlatformIO + Arduino; HAL is M5Cardputer (pulls M5Unified). Official Adv `platformio.ini` flags from M5 docs.
- Overlay is true 1:1 with pan. No scale-to-fit.
- Default `inches_per_px = 0.004140`. Settings may override. Persist `inches_per_px=0.004140` in `/sd/adv-keycopy/settings.ini` or NVS.
- Saves live in folder `adv-keycopy` at the FAT root (device path `/sd/adv-keycopy/` if the volume is mounted at `/sd`, else `/adv-keycopy`). Never write `.keycopy` to flash.
- Files are Flipper Key Copier Format v1 (`Filetype: Flipper Key Copier File`).
- Measure keys: `,`/`.` pin, `;`/`/` depth, `0`–`9` set depth, `-`/`=`/`[`/`]` pan, ESC or `` ` `` back.
- Illegal MACS or out-of-range depth: ignore the key.
- Out of scope: Wi-Fi, phone app, Flipper QR screen, UiFlow, key cutting.
- Keep `.gitignore` updated so `.pio/`, IDE folders, and build artifacts are never committed.

---

## File structure

| File | Responsibility |
| --- | --- |
| `.gitignore` | Ignore `.pio/`, IDE, OS, binaries, test junk |
| `platformio.ini` | `cardputer-adv` device env + `native` Unity env |
| `src/key_formats.h/.cpp` | 23 `KeyFormat` records, `FORMAT_NUM`, `find_format_by_name` |
| `src/key_geometry.h/.cpp` | `pin_center_inch`, `depth_change_allowed`, `build_contour` |
| `src/keycopy_io.h/.cpp` | Serialize/parse `.keycopy` text, settings.ini, filename check |
| `src/view_math.h/.cpp` | Inch→pixel, pan clamp, auto-pan, measure key mapping |
| `src/model.h` | `KeyCopierModel` shared by shell and measure view |
| `src/measure_view.h/.cpp` | Draw overlay + apply measure input (device) |
| `src/app_shell.h/.cpp` | Menus, save/load/settings/help, SD, NVS |
| `src/main.cpp` | `setup`/`loop` |
| `test/test_native/test_native.cpp` | All host Unity tests |

`README.md` and the spec already exist. Do not rewrite them unless a path or key binding changes; then update both.

---

### Task 1: Repo scaffold and `.gitignore`

**Files:**
- Create: `.gitignore` (already present in the repo — overwrite only if missing keys below)
- Create: `platformio.ini`
- Create: `src/main.cpp`
- Create: `test/test_native/test_native.cpp`

**Interfaces:**
- Consumes: nothing
- Produces: `pio test -e native` runs Unity; `pio run -e cardputer-adv` can start resolving libs (full link may wait until later tasks add sources)

- [ ] **Step 1: Write `.gitignore`**

```
# PlatformIO
.pio/
.pioenvs/
.piolibdeps/
.clang_complete
.gcc-flags.json

# Build / flash artifacts
*.elf
*.bin
*.hex
*.map
*.o
*.a
compile_commands.json

# IDE / editor
.vscode/
.idea/
*.swp
*.swo
*~
.cache/

# OS
.DS_Store
Thumbs.db

# Test output
test_results/
.pytest_cache/

# Python tooling (PlatformIO)
__pycache__/
*.pyc
.python-version

# Optional local secrets / machine paths
.env
.envrc
```

If the file already exists with this content, leave it. After every later `git add`, run `git status` and confirm `.pio/` is not listed.

- [ ] **Step 2: Write `platformio.ini`**

```ini
[platformio]
default_envs = cardputer-adv

[env:cardputer-adv]
platform = espressif32@6.7.0
board = esp32-s3-devkitc-1
framework = arduino
upload_speed = 1500000
monitor_speed = 115200
build_flags =
    -DESP32S3
    -DCORE_DEBUG_LEVEL=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
lib_deps =
    M5Cardputer=https://github.com/m5stack/M5Cardputer

[env:native]
platform = native
test_framework = unity
build_src_filter =
    +<key_formats.cpp>
    +<key_geometry.cpp>
    +<keycopy_io.cpp>
    +<view_math.cpp>
```

- [ ] **Step 3: Write a native smoke test that does not need app sources yet**

`test/test_native/test_native.cpp`:

```cpp
#include <unity.h>

void test_native_runner_works(void) {
    TEST_ASSERT_EQUAL_INT(2, 1 + 1);
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_native_runner_works);
    return UNITY_END();
}
```

- [ ] **Step 4: Write device `src/main.cpp` stub**

```cpp
#ifdef ARDUINO
#include <M5Cardputer.h>

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setCursor(4, 8);
    M5Cardputer.Display.print("Key Copier");
}

void loop() {
    M5Cardputer.update();
}
#endif
```

- [ ] **Step 5: Run native tests**

Run: `pio test -e native`

Expected: PASS (`test_native_runner_works`).

- [ ] **Step 6: Commit**

```bash
git add .gitignore platformio.ini src/main.cpp test/test_native/test_native.cpp
git status
git commit -m "chore: PlatformIO scaffold and gitignore for Adv Key Copier"
```

Confirm `git status` before commit does not list `.pio/`.

---

### Task 2: `key_formats` table

**Files:**
- Create: `src/key_formats.h`
- Create: `src/key_formats.cpp`
- Modify: `test/test_native/test_native.cpp`

**Interfaces:**
- Consumes: nothing
- Produces:
  - `enum { FORMAT_NUM = 23 };`
  - `struct KeyFormat` with fields listed below
  - `extern const KeyFormat all_formats[FORMAT_NUM];`
  - `int find_format_by_name(const char* format_name);` returns index or `-1`

- [ ] **Step 1: Write failing tests**

Add to `test/test_native/test_native.cpp` (include `"../../src/key_formats.h"` — if Unity/native include path already has `src/`, use `#include "key_formats.h"` instead; prefer `#include "key_formats.h"` and add to `platformio.ini` under `[env:native]`:

```ini
build_flags = -I src
```

Tests:

```cpp
#include "key_formats.h"

void test_format_count_is_23(void) {
    TEST_ASSERT_EQUAL_INT(23, FORMAT_NUM);
}

void test_kw1_fields(void) {
    TEST_ASSERT_EQUAL_STRING("Kwikset", all_formats[0].manufacturer);
    TEST_ASSERT_EQUAL_STRING("KW1", all_formats[0].format_name);
    TEST_ASSERT_EQUAL_INT(1, all_formats[0].sides);
    TEST_ASSERT_EQUAL_INT(1, all_formats[0].stop);
    TEST_ASSERT_EQUAL_INT(5, all_formats[0].pin_num);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.247, all_formats[0].first_pin_inch);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.847, all_formats[0].last_pin_inch);
    TEST_ASSERT_EQUAL_INT(1, all_formats[0].min_depth_ind);
    TEST_ASSERT_EQUAL_INT(7, all_formats[0].max_depth_ind);
    TEST_ASSERT_EQUAL_INT(4, all_formats[0].macs);
}

void test_sc4_pin_count(void) {
    TEST_ASSERT_EQUAL_STRING("SC4", all_formats[1].format_name);
    TEST_ASSERT_EQUAL_INT(6, all_formats[1].pin_num);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.231, all_formats[1].first_pin_inch);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.012, all_formats[1].last_pin_inch);
}

void test_h75_is_double_sided(void) {
    int i = find_format_by_name("H75");
    TEST_ASSERT_TRUE(i >= 0);
    TEST_ASSERT_EQUAL_INT(2, all_formats[i].sides);
    TEST_ASSERT_EQUAL_INT(2, all_formats[i].stop);
    TEST_ASSERT_EQUAL_INT(8, all_formats[i].pin_num);
}

void test_find_format_unknown(void) {
    TEST_ASSERT_EQUAL_INT(-1, find_format_by_name("NOPE"));
}
```

Add `RUN_TEST` lines for each. Do not implement `find_format_by_name` yet.

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native`

Expected: FAIL compile (`key_formats.h` not found) or link error.

- [ ] **Step 3: Write `src/key_formats.h`**

```cpp
#ifndef KEY_FORMATS_H
#define KEY_FORMATS_H

enum { FORMAT_NUM = 23 };

typedef struct {
    const char* manufacturer;
    const char* format_name;
    const char* format_link;
    int sides;
    int stop;
    double first_pin_inch;
    double last_pin_inch;
    double pin_increment_inch;
    int pin_num;
    double pin_width_inch;
    double drill_angle;
    double elbow_inch;
    double uncut_depth_inch;
    double deepest_depth_inch;
    double depth_step_inch;
    int min_depth_ind;
    int max_depth_ind;
    int macs;
    int clearance;
} KeyFormat;

extern const KeyFormat all_formats[FORMAT_NUM];

int find_format_by_name(const char* format_name);

#endif
```

- [ ] **Step 4: Write `src/key_formats.cpp`**

Copy every numeric field from Flipper `key_formats.c` (v1.5). Set `sides = 1` and `stop = 1` unless Flipper set `2`. Order must match Flipper so index `0` is KW1 and `1` is SC4:

```cpp
#include "key_formats.h"
#include <string.h>

const KeyFormat all_formats[FORMAT_NUM] = {
    {"Kwikset", "KW1", "https://lsamichigan.org/Tech/Kwikset_KeySpecs.pdf", 1, 1,
     0.247, 0.847, 0.15, 5, 0.084, 90, 0.15, 0.329, 0.191, 0.023, 1, 7, 4, 3},
    {"Schlage", "SC4", "https://lsamichigan.org/Tech/SCHLAGE_KeySpecs.pdf", 1, 1,
     0.231, 1.012, 0.1562, 6, 0.031, 90, 0.1, 0.335, 0.2, 0.015, 0, 9, 7, 8},
    {"Arrow", "AR4", "C2", 1, 1, 0.265, 1.040, 0.155, 6, 0.060, 90, 0.1, 0.312, 0.186, 0.014, 0, 9, 6, 7},
    {"Master Lock", "M1", "C35", 1, 1, 0.185, 0.689, 0.126, 5, 0.039, 90, 0.1, 0.276, 0.171, 0.015, 0, 7, 7, 6},
    {"American", "AM7", "C80", 1, 1, 0.157, 0.781, 0.125, 6, 0.039, 90, 0.1, 0.283, 0.173, 0.016, 1, 8, 7, 5},
    {"Yale", "Y2", "C57", 1, 1, 0.200, 1.025, 0.165, 6, 0.054, 90, 0.1, 0.320, 0.149, 0.019, 0, 9, 9, 4},
    {"Yale", "Y11", "CX55", 1, 1, 0.124, 0.502, 0.095, 5, 0.039, 90, 0.1, 0.246, 0.167, 0.020, 1, 5, 7, 3},
    {"Sargent", "S22", "C44", 1, 1, 0.216, 0.996, 0.156, 6, 0.063, 90, 0.1, 0.328, 0.148, 0.020, 1, 10, 7, 5},
    {"National", "NA25", "C40", 1, 1, 0.250, 0.874, 0.156, 5, 0.039, 90, 0.1, 0.304, 0.191, 0.012, 0, 9, 7, 8},
    {"Corbin", "CO88", "C14", 1, 1, 0.250, 1.030, 0.156, 6, 0.047, 90, 0.1, 0.343, 0.217, 0.014, 1, 10, 7, 8},
    {"Lockwood", "LW4", "", 1, 1, 0.245, 0.870, 0.1562, 5, 0.031, 90, 0.1, 0.344, 0.203, 0.014, 0, 9, 9, 8},
    {"Lockwood", "LW5", "", 1, 1, 0.245, 1.0262, 0.1562, 6, 0.031, 90, 0.1, 0.344, 0.203, 0.014, 0, 9, 9, 8},
    {"National", "NA12", "C39", 1, 1, 0.150, 0.710, 0.140, 5, 0.039, 90, 0.1, 0.270, 0.157, 0.013, 0, 9, 7, 8},
    {"Russwin", "RU45", "CX6", 1, 1, 0.250, 1.030, 0.156, 6, 0.053, 90, 0.1, 0.343, 0.203, 0.028, 1, 6, 5, 3},
    {"Weiser", "WR3", "https://www.lockwiki.com/index.php/Weiser_Classic", 1, 1,
     0.237, 0.861, 0.156, 5, 0.090, 90, 0.150, 0.315, 0.153, 0.018, 0, 10, 6, 3},
    {"Ford", "H75", "CX101", 2, 2, 0.201, 0.845, 0.092, 8, 0.039, 90, 0.201, 0.354, 0.254, 0.025, 1, 5, 5, 2},
    {"Chevrolet", "B102", "", 2, 2, 0.205, 1.037, 0.093, 10, 0.039, 90, 0.205, 0.315, 0.161, 0.026, 1, 4, 5, 2},
    {"Dodge", "Y159", "CX102", 2, 2, 0.297, 0.941, 0.092, 8, 0.039, 90, 0.297, 0.339, 0.197, 0.047, 1, 4, 5, 1},
    {"Kawasaki", "KA14", "CMC50", 2, 1, 0.098, 0.591, 0.098, 6, 0.039, 90, 0.1, 0.258, 0.198, 0.020, 1, 4, 4, 3},
    {"Suzuki", "SUZ18", "X241", 2, 1, 0.16, 0.73, 0.095, 7, 0.045, 90, 0.1, 0.28, 0.22, 0.020, 1, 4, 4, 3},
    {"Yamaha", "YM63", "CMC71", 2, 1, 0.157, 0.748, 0.098, 7, 0.039, 90, 0.1, 0.295, 0.236, 0.020, 1, 4, 4, 3},
    {"Best (A2)", "SFIC", "C3", 1, 2, 0.250, 0.998, 0.149, 6, 0.051, 90, 0.081, 0.318, 0.206, 0.025, 0, 9, 5, 3},
    {"RV (FIC,GL,Bauer)", "RV", "Card", 2, 1, 0.126, 0.504, 0.094, 5, 0.039, 90, 0.126, 0.260, 0.181, 0.040, 1, 3, 3, 1},
};

int find_format_by_name(const char* format_name) {
    if (!format_name) return -1;
    for (int i = 0; i < FORMAT_NUM; i++) {
        if (strcmp(all_formats[i].format_name, format_name) == 0) return i;
    }
    return -1;
}
```

- [ ] **Step 5: Run tests**

Run: `pio test -e native`

Expected: all format tests PASS.

- [ ] **Step 6: Commit**

```bash
git add src/key_formats.h src/key_formats.cpp test/test_native/test_native.cpp platformio.ini
git commit -m "feat: add Flipper 1.5 key format table"
```

---

### Task 3: MACS and depth limits

**Files:**
- Create: `src/key_geometry.h`
- Create: `src/key_geometry.cpp` (MACS only in this task)
- Modify: `test/test_native/test_native.cpp`

**Interfaces:**
- Consumes: `KeyFormat` from `key_formats.h`
- Produces: `bool depth_change_allowed(const KeyFormat& format, const uint8_t* depths, int pin_index, uint8_t new_depth);`
  - `pin_index` is 0-based
  - `depths` has at least `format.pin_num` entries
  - Reject if `new_depth` is outside `[min_depth_ind, max_depth_ind]`
  - Reject if `abs(new_depth - neighbor) > format.macs` for the previous pin (when `pin_index > 0`) and/or the next pin (when `pin_index < pin_num - 1`)
  - Same depth as current is allowed

- [ ] **Step 1: Write failing tests**

```cpp
#include "key_geometry.h"

static void fill_kw1_min(uint8_t* d) {
    for (int i = 0; i < 6; i++) d[i] = 1;
}

void test_macs_rejects_out_of_range(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 0, 0));
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 0, 8));
}

void test_macs_allows_legal_first_pin(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    d[1] = 1;
    TEST_ASSERT_TRUE(depth_change_allowed(all_formats[0], d, 0, 5));
}

void test_macs_rejects_illegal_adjacent(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    d[1] = 1;
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 0, 6));
}

void test_macs_middle_pin_checks_both_neighbors(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    d[0] = 1;
    d[1] = 1;
    d[2] = 1;
    TEST_ASSERT_TRUE(depth_change_allowed(all_formats[0], d, 1, 5));
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 1, 6));
}
```

KW1 `macs = 4`, so `|6-1| = 5 > 4` is illegal and `|5-1| = 4` is legal.

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native`

Expected: FAIL compile (`depth_change_allowed` undeclared).

- [ ] **Step 3: Write header + implementation**

`src/key_geometry.h`:

```cpp
#ifndef KEY_GEOMETRY_H
#define KEY_GEOMETRY_H

#include "key_formats.h"
#include <stdint.h>

typedef struct {
    double x0, y0, x1, y1;
} Segment;

double pin_center_inch(const KeyFormat& format, int pin_1based);

bool depth_change_allowed(const KeyFormat& format, const uint8_t* depths, int pin_index,
                          uint8_t new_depth);

int build_contour(const KeyFormat& format, const uint8_t* depths, Segment* out, int out_cap);

#endif
```

`src/key_geometry.cpp` (MACS + stub contour for now):

```cpp
#include "key_geometry.h"
#include <stdlib.h>

double pin_center_inch(const KeyFormat& format, int pin_1based) {
    return format.first_pin_inch + (double)(pin_1based - 1) * format.pin_increment_inch;
}

bool depth_change_allowed(const KeyFormat& format, const uint8_t* depths, int pin_index,
                          uint8_t new_depth) {
    if (pin_index < 0 || pin_index >= format.pin_num) return false;
    if (new_depth < (uint8_t)format.min_depth_ind || new_depth > (uint8_t)format.max_depth_ind) {
        return false;
    }
    if (pin_index > 0) {
        int d = (int)new_depth - (int)depths[pin_index - 1];
        if (d < 0) d = -d;
        if (d > format.macs) return false;
    }
    if (pin_index < format.pin_num - 1) {
        int d = (int)new_depth - (int)depths[pin_index + 1];
        if (d < 0) d = -d;
        if (d > format.macs) return false;
    }
    return true;
}

int build_contour(const KeyFormat& format, const uint8_t* depths, Segment* out, int out_cap) {
    (void)format;
    (void)depths;
    (void)out;
    (void)out_cap;
    return 0;
}
```

- [ ] **Step 4: Run tests**

Run: `pio test -e native`

Expected: MACS tests PASS. (`build_contour` is a stub; contour tests come next.)

- [ ] **Step 5: Commit**

```bash
git add src/key_geometry.h src/key_geometry.cpp test/test_native/test_native.cpp
git commit -m "feat: enforce MACS and depth range before pin edits"
```

---

### Task 4: Inch-space contour

**Files:**
- Modify: `src/key_geometry.cpp`
- Modify: `test/test_native/test_native.cpp`

**Interfaces:**
- Consumes: `KeyFormat`, `depths[0 .. pin_num-1]` (and optional `depths[pin_num]` padding unused by this function)
- Produces: `build_contour` writes line segments in inches. `x = 0` is the shoulder. `y = 0` is the uncut top. `y` increases downward into the cut. Single-sided blade bottom is `y = uncut_depth_inch`. Double-sided (`sides == 2`) bottom uncut edge is the same `y = uncut_depth_inch`; bottom cuts move toward smaller `y`.
- Also produces `pin_center_inch(format, pin_1based)`.

Coordinate rules (Flipper draw loop, inches instead of pixels):

- Pin flat at pin `i` (1-based): horizontal from `cx - pin_width/2` to `cx + pin_width/2` at `y = cut`, where `cx = pin_center_inch`, `cut = (depth[i-1] - min_depth_ind) * depth_step_inch`.
- First-pin shoulder: `(0, 0)` to `(cx - pin_width/2 - cut, 0)`.
- Single-sided blade bottom: `(0, uncut_depth_inch)` to `(last_pin_inch + elbow_inch, uncut_depth_inch)`.
- Elbow: `(last_pin_inch + elbow_inch, uncut_depth_inch)` to `(last_pin_inch + 2*elbow_inch, uncut_depth_inch - elbow_inch)`.
- Tip-stop (`stop == 2`): vertical at `x = last_pin_inch + elbow_inch` from `y = 0` to `y = uncut_depth_inch`.
- Drill slopes: `tangent = tan(((180 - drill_angle) / 2) * pi / 180)`. If `(last_cut_index + this_cut_index) > clearance`, connect neighboring cuts with the Flipper intersection split; otherwise draw a shoulder-style slope `cut` inches wide at 1:1 x/y using `tangent` on the vertical component (`cut * tangent` for 90° drill is `cut`).
- Double-sided: also emit the mirrored flats and slopes from `y = uncut_depth_inch` upward.

Keep a helper:

```cpp
static void add_seg(Segment* out, int cap, int* n, double x0, double y0, double x1, double y1) {
    if (*n < 0 || *n >= cap) return;
    out[*n].x0 = x0;
    out[*n].y0 = y0;
    out[*n].x1 = x1;
    out[*n].y1 = y1;
    (*n)++;
}
```

Port the Flipper `key_copier_view_measure_draw_callback` loop from [zinongli/KeyCopier `key_copier.c`](https://github.com/zinongli/KeyCopier/blob/main/key_copier.c) lines 246–491. Replace every ` / inches_per_px` and `round(...)` with raw inches. Replace canvas `62` with `uncut_depth_inch`. Do not call any display API.

- [ ] **Step 1: Write failing tests**

```cpp
static int count_near(const Segment* segs, int n, double x0, double y0, double x1, double y1) {
    int c = 0;
    for (int i = 0; i < n; i++) {
        int match = (fabs(segs[i].x0 - x0) < 0.001 && fabs(segs[i].y0 - y0) < 0.001 &&
                     fabs(segs[i].x1 - x1) < 0.001 && fabs(segs[i].y1 - y1) < 0.001);
        int rev = (fabs(segs[i].x0 - x1) < 0.001 && fabs(segs[i].y0 - y1) < 0.001 &&
                   fabs(segs[i].x1 - x0) < 0.001 && fabs(segs[i].y1 - y0) < 0.001);
        if (match || rev) c++;
    }
    return c;
}

void test_pin_centers_kw1_sc4(void) {
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.247, pin_center_inch(all_formats[0], 1));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.847, pin_center_inch(all_formats[0], 5));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.231, pin_center_inch(all_formats[1], 1));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.012, pin_center_inch(all_formats[1], 6));
}

void test_kw1_shoulder_and_elbow(void) {
    uint8_t d[6] = {1, 1, 1, 1, 1, 1};
    Segment segs[128];
    int n = build_contour(all_formats[0], d, segs, 128);
    TEST_ASSERT_TRUE(n > 4);
    double half = 0.084 / 2.0;
    TEST_ASSERT_TRUE(count_near(segs, n, 0.0, 0.0, 0.247 - half, 0.0) >= 1);
    double level = 0.847 + 0.15;
    TEST_ASSERT_TRUE(count_near(segs, n, 0.0, 0.329, level, 0.329) >= 1);
    TEST_ASSERT_TRUE(count_near(segs, n, level, 0.329, level + 0.15, 0.329 - 0.15) >= 1);
}

void test_h75_has_bottom_edge(void) {
    int hi = find_format_by_name("H75");
    uint8_t d[16];
    for (int i = 0; i < 16; i++) d[i] = (uint8_t)all_formats[hi].min_depth_ind;
    Segment segs[256];
    int n = build_contour(all_formats[hi], d, segs, 256);
    int bottomish = 0;
    double uncut = all_formats[hi].uncut_depth_inch;
    for (int i = 0; i < n; i++) {
        if (fabs(segs[i].y0 - uncut) < 0.002 && fabs(segs[i].y1 - uncut) < 0.002) bottomish++;
    }
    TEST_ASSERT_TRUE(bottomish >= 1);
}
```

Add `#include <math.h>`.

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native`

Expected: FAIL `test_kw1_shoulder_and_elbow` (`n > 4` is false because stub returns 0).

- [ ] **Step 3: Implement `build_contour`**

Replace the stub. Use this structure (fill the slope branches from Flipper; all units inches):

```cpp
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int build_contour(const KeyFormat& format, const uint8_t* depths, Segment* out, int out_cap) {
    int n = 0;
    const double half = format.pin_width_inch / 2.0;
    const double step = format.pin_increment_inch;
    const double drill_rad = (180.0 - format.drill_angle) / 2.0 / 180.0 * M_PI;
    const double tangent = tan(drill_rad);
    const double top = 0.0;
    const double bottom = format.uncut_depth_inch;
    const double level_x = format.last_pin_inch + format.elbow_inch;
    double post_extra = 0.0;
    double pre_extra = 0.0;
    double bot_post = 0.0;
    double bot_pre = 0.0;

    for (int pin = 1; pin <= format.pin_num; pin++) {
        const double cx = pin_center_inch(format, pin);
        const int this_i = (int)depths[pin - 1] - format.min_depth_ind;
        const double cut = (double)this_i * format.depth_step_inch;
        int last_i = (pin == 1) ? 0 : ((int)depths[pin - 2] - format.min_depth_ind);
        int next_i = (pin == format.pin_num) ? 0 : ((int)depths[pin] - format.min_depth_ind);

        add_seg(out, out_cap, &n, cx - half, top + cut, cx + half, top + cut);

        if (format.sides == 2) {
            add_seg(out, out_cap, &n, cx - half, bottom - cut, cx + half, bottom - cut);
        }

        if (pin == 1) {
            add_seg(out, out_cap, &n, 0.0, top, cx - half - cut, top);
            last_i = 0;
            pre_extra = (cut + half > 0.0) ? (cut + half) : 0.0;
            if (format.sides != 2) {
                add_seg(out, out_cap, &n, 0.0, bottom, level_x, bottom);
            } else {
                add_seg(out, out_cap, &n, 0.0, bottom, cx - half - cut, bottom);
            }
        }

        if ((last_i + this_i) > format.clearance) {
            if (pin != 1) {
                double v = step - post_extra;
                if (v < half) v = half;
                if (v > step - half) v = step - half;
                pre_extra = v;
            }
            double y = (cut - (pre_extra - half)) * tangent;
            if (y < 0.0) y = 0.0;
            add_seg(out, out_cap, &n, cx - pre_extra, top + y, cx - half, top + cut * tangent);
        } else {
            const double last_cut = (double)last_i * format.depth_step_inch;
            const double down_x = cx - half - cut;
            add_seg(out, out_cap, &n, down_x, top, cx - half, top + cut * tangent);
            double mid = cx - step + half + last_cut;
            if (mid > down_x) mid = down_x;
            add_seg(out, out_cap, &n, mid, top, down_x, top);
        }

        if ((this_i + next_i) > format.clearance) {
            double product = ((double)this_i / (double)(this_i + next_i)) * step;
            post_extra = product;
            if (post_extra < half) post_extra = half;
            if (post_extra > step - half) post_extra = step - half;
            double y = cut - (post_extra - half) * tangent;
            if (y < 0.0) y = 0.0;
            add_seg(out, out_cap, &n, cx + half, top + cut, cx + post_extra, top + y);
        } else {
            add_seg(out, out_cap, &n, cx + half, top + cut * tangent, cx + half + cut, top);
        }

        if (format.sides == 2) {
            if (pin == 1) {
                last_i = 0;
                bot_pre = (cut + half > 0.0) ? (cut + half) : 0.0;
            }
            if ((last_i + this_i) > format.clearance) {
                if (pin != 1) {
                    double v = step - bot_post;
                    if (v < half) v = half;
                    if (v > step - half) v = step - half;
                    bot_pre = v;
                }
                double y = (cut - (bot_pre - half)) * tangent;
                if (y < 0.0) y = 0.0;
                add_seg(out, out_cap, &n, cx - bot_pre, bottom - y, cx - half, bottom - cut * tangent);
            } else {
                const double last_cut = (double)last_i * format.depth_step_inch;
                const double up_x = cx - half - cut;
                add_seg(out, out_cap, &n, up_x, bottom, cx - half, bottom - cut * tangent);
                double mid = cx - step + half + last_cut;
                if (mid > up_x) mid = up_x;
                add_seg(out, out_cap, &n, mid, bottom, up_x, bottom);
            }
            if ((this_i + next_i) > format.clearance) {
                double product = ((double)this_i / (double)(this_i + next_i)) * step;
                bot_post = product;
                if (bot_post < half) bot_post = half;
                if (bot_post > step - half) bot_post = step - half;
                double y = cut - (bot_post - half) * tangent;
                if (y < 0.0) y = 0.0;
                add_seg(out, out_cap, &n, cx + half, bottom - cut, cx + bot_post, bottom - y);
            } else {
                add_seg(out, out_cap, &n, cx + half, bottom - cut * tangent, cx + half + cut, bottom);
            }
        }
    }

    add_seg(out, out_cap, &n, level_x, bottom, level_x + format.elbow_inch, bottom - format.elbow_inch);
    if (format.stop == 2) {
        add_seg(out, out_cap, &n, level_x, top, level_x, bottom);
    }
    return n;
}
```

Do not emit zero-length segments. `measure_view` may draw a short vertical at inch x=0 for the shoulder tick.

- [ ] **Step 4: Run tests**

Run: `pio test -e native`

Expected: contour tests PASS. If shoulder x1 is `0.247 - 0.042 = 0.205` and your cut at min depth is `0`, that must match. If a test fails on elbow endpoints, print `n` and the segments and fix the last `add_seg` only — do not scale.

- [ ] **Step 5: Commit**

```bash
git add src/key_geometry.cpp test/test_native/test_native.cpp
git commit -m "feat: build 1:1 key contours in inches"
```

---

### Task 5: `.keycopy` text and settings.ini

**Files:**
- Create: `src/keycopy_io.h`
- Create: `src/keycopy_io.cpp`
- Modify: `test/test_native/test_native.cpp`

**Interfaces:**
- Consumes: `KeyFormat`, `all_formats`, `find_format_by_name`
- Produces:
  - `bool keycopy_name_ok(const char* name);`
  - `bool keycopy_serialize(const KeyFormat& format, const uint8_t* depths, char* buf, size_t buf_len);`
  - `bool keycopy_parse(const char* text, int* format_index, uint8_t* depths, int depths_cap);`
  - `bool parse_settings_ini(const char* text, double* out_pitch);`
  - `int format_settings_ini(double pitch, char* buf, size_t buf_len);` returns bytes written excluding NUL, or `-1`
  - `#define DEFAULT_INCHES_PER_PX 0.004140`

Serialize exact lines (LF):

```
Filetype: Flipper Key Copier File
Version: 1
Manufacturer: <manufacturer>
Format Name: <format_name>
Data Sheet: <format_link>
Number of Pins: <pin_num>
Maximum Adjacent Cut Specification (MACS): <macs>
Bitting Pattern: d-d-d
```

`keycopy_parse` requires `Format Name` and `Bitting Pattern`. Bitting must have length `2 * pin_num - 1` with digits at even indexes and `-` at odd indexes. Set `depths[i] = text_digit - '0'`. Unknown format name → `false` and do not write `depths`. `depths_cap` must be `>= pin_num`.

`keycopy_name_ok`: false if NULL, empty, `.`, `..`, or any character in `/\:*?"<>|`.

`parse_settings_ini`: find `inches_per_px=` and `strtod`. False if missing or `pitch <= 0`.

- [ ] **Step 1: Write failing tests**

```cpp
#include "keycopy_io.h"

void test_keycopy_round_trip_kw1(void) {
    uint8_t in[5] = {1, 2, 3, 4, 5};
    char buf[512];
    TEST_ASSERT_TRUE(keycopy_serialize(all_formats[0], in, buf, sizeof(buf)));
    TEST_ASSERT_NOT_NULL(strstr(buf, "Filetype: Flipper Key Copier File"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "Format Name: KW1"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "Bitting Pattern: 1-2-3-4-5"));
    int idx = -1;
    uint8_t out[8] = {0};
    TEST_ASSERT_TRUE(keycopy_parse(buf, &idx, out, 8));
    TEST_ASSERT_EQUAL_INT(0, idx);
    TEST_ASSERT_EQUAL_UINT8(1, out[0]);
    TEST_ASSERT_EQUAL_UINT8(5, out[4]);
}

void test_keycopy_rejects_unknown_format(void) {
    const char* t =
        "Filetype: Flipper Key Copier File\n"
        "Version: 1\n"
        "Format Name: NOPE\n"
        "Bitting Pattern: 1-2-3-4-5\n";
    int idx = 99;
    uint8_t out[8] = {9, 9, 9, 9, 9, 9, 9, 9};
    TEST_ASSERT_FALSE(keycopy_parse(t, &idx, out, 8));
    TEST_ASSERT_EQUAL_INT(99, idx);
    TEST_ASSERT_EQUAL_UINT8(9, out[0]);
}

void test_keycopy_rejects_short_bitting(void) {
    const char* t = "Format Name: KW1\nBitting Pattern: 1-2-3\n";
    int idx = 0;
    uint8_t out[8] = {0};
    TEST_ASSERT_FALSE(keycopy_parse(t, &idx, out, 8));
}

void test_keycopy_name_rules(void) {
    TEST_ASSERT_TRUE(keycopy_name_ok("house"));
    TEST_ASSERT_FALSE(keycopy_name_ok(""));
    TEST_ASSERT_FALSE(keycopy_name_ok("."));
    TEST_ASSERT_FALSE(keycopy_name_ok(".."));
    TEST_ASSERT_FALSE(keycopy_name_ok("a/b"));
    TEST_ASSERT_FALSE(keycopy_name_ok("a:b"));
}

void test_settings_ini_round_trip(void) {
    char buf[64];
    TEST_ASSERT_TRUE(format_settings_ini(0.004140, buf, sizeof(buf)) > 0);
    TEST_ASSERT_NOT_NULL(strstr(buf, "inches_per_px=0.004140"));
    double p = 0;
    TEST_ASSERT_TRUE(parse_settings_ini(buf, &p));
    TEST_ASSERT_DOUBLE_WITHIN(0.0000001, 0.004140, p);
}

void test_settings_ini_rejects_non_positive(void) {
    double p = 1;
    TEST_ASSERT_FALSE(parse_settings_ini("inches_per_px=0\n", &p));
    TEST_ASSERT_FALSE(parse_settings_ini("nope\n", &p));
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native`

Expected: FAIL compile (`keycopy_io.h` not found).

- [ ] **Step 3: Implement `keycopy_io`**

`src/keycopy_io.h`:

```cpp
#ifndef KEYCOPY_IO_H
#define KEYCOPY_IO_H

#include "key_formats.h"
#include <stddef.h>
#include <stdint.h>

#define DEFAULT_INCHES_PER_PX 0.004140
#define KEYCOPY_DIR_NAME "adv-keycopy"

bool keycopy_name_ok(const char* name);
bool keycopy_serialize(const KeyFormat& format, const uint8_t* depths, char* buf, size_t buf_len);
bool keycopy_parse(const char* text, int* format_index, uint8_t* depths, int depths_cap);
bool parse_settings_ini(const char* text, double* out_pitch);
int format_settings_ini(double pitch, char* buf, size_t buf_len);

#endif
```

`src/keycopy_io.cpp` — implement with `snprintf` / line scan via `strstr` and newline slicing. For parse, walk lines; on `Format Name:` skip one space after `:`; on `Bitting Pattern:` take the rest of the line (strip `\r`). After both found, `find_format_by_name`, then validate bitting length `2 * pin_num - 1`, then fill depths. If anything fails, return false without copying depths (parse into locals first, then memcpy).

`format_settings_ini`: `snprintf(buf, buf_len, "inches_per_px=%.6f\n", pitch);`

`parse_settings_ini`: `const char* p = strstr(text, "inches_per_px=");` then `strtod(p + 14, &end);` require `pitch > 0`.

- [ ] **Step 4: Run tests**

Run: `pio test -e native`

Expected: all keycopy and settings tests PASS.

- [ ] **Step 5: Commit**

```bash
git add src/keycopy_io.h src/keycopy_io.cpp test/test_native/test_native.cpp
git commit -m "feat: serialize Flipper-compatible keycopy and settings"
```

---

### Task 6: View math and measure keys

**Files:**
- Create: `src/view_math.h`
- Create: `src/view_math.cpp`
- Modify: `test/test_native/test_native.cpp`

**Interfaces:**
- Consumes: nothing (pure pixel/inch and char mapping)
- Produces:

```cpp
#define SCREEN_W 240
#define SCREEN_H 135
#define ORIGIN_X 0
#define ORIGIN_Y 18

int inch_to_px(double inch, double inches_per_px);

typedef struct {
    int x;
    int y;
} Pixel;

Pixel world_to_screen(double inch_x, double inch_y, double inches_per_px, int origin_x,
                      int origin_y, int pan_x, int pan_y);

void clamp_pan(int* pan_x, int* pan_y, int min_x, int max_x, int min_y, int max_y, int view_w,
               int view_h);

void autopan_to_pin(int pin_px_x, int pin_px_y, int* pan_x, int* pan_y, int view_w, int view_h,
                    int margin);

typedef enum {
    MeasureAction_None,
    MeasureAction_PinPrev,
    MeasureAction_PinNext,
    MeasureAction_Shallower,
    MeasureAction_Deeper,
    MeasureAction_SetDepth,
    MeasureAction_PanLeft,
    MeasureAction_PanRight,
    MeasureAction_PanUp,
    MeasureAction_PanDown,
    MeasureAction_Back
} MeasureAction;

typedef struct {
    MeasureAction action;
    uint8_t digit;
} MeasureInput;

MeasureInput measure_input_from_char(char c);
```

Rules:

- `inch_to_px` = `lround(inch / inches_per_px)`
- `world_to_screen`: `x = origin_x + inch_to_px(inch_x) - pan_x`, same for y
- `clamp_pan`: after clamp, the rect `[min,max]` still intersects the view. If contour is smaller than the view, pin pan so the contour stays on screen (min aligned, or zero). If larger, `pan_x` in `[min_x, max_x - view_w]` (and the same for y). If `max - min <= view`, set pan so `min` maps toward 0.
- `autopan_to_pin`: pin is at `pin_px_* - pan`. If that x is outside `[margin, view_w - margin]`, set `pan_x` so it lands at `margin` or `view_w - margin`. Same for y.
- `measure_input_from_char`:
  - `,` → PinPrev; `.` → PinNext; `;` → Shallower; `/` → Deeper
  - `0`–`9` → SetDepth with that digit
  - `-` PanLeft; `=` PanRight; `[` PanUp; `]` PanDown
  - `` ` `` or `27` (ESC if passed as char) → Back
  - else None

- [ ] **Step 1: Write failing tests**

```cpp
#include "view_math.h"

void test_inch_to_px_default_pitch(void) {
    TEST_ASSERT_EQUAL_INT(0, inch_to_px(0.0, DEFAULT_INCHES_PER_PX));
    TEST_ASSERT_EQUAL_INT(60, inch_to_px(0.2484, DEFAULT_INCHES_PER_PX));
}

void test_world_to_screen_subtracts_pan(void) {
    Pixel p = world_to_screen(0.2484, 0.0, DEFAULT_INCHES_PER_PX, 0, 20, 10, 0);
    TEST_ASSERT_EQUAL_INT(50, p.x);
    TEST_ASSERT_EQUAL_INT(20, p.y);
}

void test_clamp_pan_long_key(void) {
    int pan_x = 5000;
    int pan_y = 0;
    clamp_pan(&pan_x, &pan_y, 0, 300, 0, 80, 240, 135);
    TEST_ASSERT_EQUAL_INT(60, pan_x);
}

void test_autopan_brings_pin_on_screen(void) {
    int pan_x = 0;
    int pan_y = 0;
    autopan_to_pin(280, 40, &pan_x, &pan_y, 240, 135, 16);
    Pixel shown = {280 - pan_x, 40 - pan_y};
    TEST_ASSERT_TRUE(shown.x >= 16 && shown.x <= 240 - 16);
}

void test_measure_keys(void) {
    TEST_ASSERT_EQUAL_INT(MeasureAction_PinPrev, measure_input_from_char(',').action);
    TEST_ASSERT_EQUAL_INT(MeasureAction_PinNext, measure_input_from_char('.').action);
    TEST_ASSERT_EQUAL_INT(MeasureAction_Shallower, measure_input_from_char(';').action);
    TEST_ASSERT_EQUAL_INT(MeasureAction_Deeper, measure_input_from_char('/').action);
    MeasureInput s = measure_input_from_char('3');
    TEST_ASSERT_EQUAL_INT(MeasureAction_SetDepth, s.action);
    TEST_ASSERT_EQUAL_UINT8(3, s.digit);
    TEST_ASSERT_EQUAL_INT(MeasureAction_PanLeft, measure_input_from_char('-').action);
    TEST_ASSERT_EQUAL_INT(MeasureAction_Back, measure_input_from_char('`').action);
}
```

`0.2484 / 0.004140 = 60` exactly.

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native`

Expected: FAIL compile.

- [ ] **Step 3: Implement `view_math`**

`inch_to_px`:

```cpp
#include <math.h>
int inch_to_px(double inch, double inches_per_px) {
    if (inches_per_px <= 0.0) inches_per_px = DEFAULT_INCHES_PER_PX;
    return (int)lround(inch / inches_per_px);
}
```

`clamp_pan` for x (mirror for y):

```cpp
static int clamp_axis(int pan, int minv, int maxv, int view) {
    int span = maxv - minv;
    if (span <= view) return minv;
    int max_pan = maxv - view;
    if (pan < minv) return minv;
    if (pan > max_pan) return max_pan;
    return pan;
}
```

`autopan_to_pin`:

```cpp
void autopan_to_pin(int pin_px_x, int pin_px_y, int* pan_x, int* pan_y, int view_w, int view_h,
                    int margin) {
    int sx = pin_px_x - *pan_x;
    if (sx < margin) *pan_x = pin_px_x - margin;
    if (sx > view_w - margin) *pan_x = pin_px_x - (view_w - margin);
    int sy = pin_px_y - *pan_y;
    if (sy < margin) *pan_y = pin_px_y - margin;
    if (sy > view_h - margin) *pan_y = pin_px_y - (view_h - margin);
}
```

Include `keycopy_io.h` for `DEFAULT_INCHES_PER_PX` or redefine the same `0.004140` in `view_math.h` as `DEFAULT_INCHES_PER_PX` only in `keycopy_io.h` and include it — **do not** define the macro in two headers. `view_math.h` includes `keycopy_io.h` or `view_math.cpp` includes it. Prefer putting `#define DEFAULT_INCHES_PER_PX 0.004140` only in `keycopy_io.h` and `#include "keycopy_io.h"` from `view_math.h`.

- [ ] **Step 4: Run tests**

Run: `pio test -e native`

Expected: PASS. If `test_inch_to_px_default_pitch` is off by 1, use `0.2484` / `0.004140` and `lround`.

- [ ] **Step 5: Commit**

```bash
git add src/view_math.h src/view_math.cpp test/test_native/test_native.cpp
git commit -m "feat: map inches to Adv pixels and measure keys"
```

---

### Task 7: Model and measure view

**Files:**
- Create: `src/model.h`
- Create: `src/measure_view.h`
- Create: `src/measure_view.cpp`

**Interfaces:**
- Consumes: `KeyFormat`, `build_contour`, `depth_change_allowed`, `pin_center_inch`, `world_to_screen`, `inch_to_px`, `clamp_pan`, `autopan_to_pin`, `measure_input_from_char`
- Produces:
  - `void model_init(KeyCopierModel* m, int format_index);` — copies `all_formats[format_index]`, `malloc` or static `depth[16]`, fills `min_depth_ind`, `pin_slc = 0`, `pan_x = pan_y = 0`, `inches_per_px = DEFAULT_INCHES_PER_PX`, `data_loaded = false`, `name[0] = 0`
  - `void model_apply_measure_input(KeyCopierModel* m, MeasureInput in);`
  - `void measure_view_draw(KeyCopierModel* m);` — device only
  - `void measure_view_handle_char(KeyCopierModel* m, char c);`

`src/model.h`:

```cpp
#ifndef MODEL_H
#define MODEL_H

#include "key_formats.h"
#include <stdint.h>
#include <stdbool.h>

#define MODEL_MAX_PINS 16
#define MODEL_NAME_LEN 32

typedef struct {
    int format_index;
    KeyFormat format;
    uint8_t depth[MODEL_MAX_PINS];
    int pin_slc;
    int pan_x;
    int pan_y;
    double inches_per_px;
    bool data_loaded;
    char name[MODEL_NAME_LEN];
} KeyCopierModel;

void model_init(KeyCopierModel* m, int format_index);
void model_apply_measure_input(KeyCopierModel* m, MeasureInput in);

#endif
```

`model.h` includes `view_math.h` for `MeasureInput`.

`model_init` lives in `measure_view.cpp` or a tiny `src/model.cpp`. Put it in `src/model.cpp` and add `+<model.cpp>` to `[env:native] build_src_filter` if you add a host test; otherwise only the device env compiles it. **Add a host test** `test_model_digit_respects_macs` in `test_native` and add `model.cpp` to the native filter.

`model_apply_measure_input` (in `model.cpp`, no Arduino):

- PinPrev/Next: clamp `pin_slc` to `[0, pin_num-1]`, then `autopan_to_pin` using `inch_to_px(pin_center_inch(format, pin_slc+1), inches_per_px)` as `pin_px_x` and `origin_y` as `pin_px_y` (use `origin_x = 0`, `origin_y = 18` constants `#define ORIGIN_X 0` `#define ORIGIN_Y 18` in `view_math.h`).
- Shallower: `new = depth[pin_slc] - 1`; if `depth_change_allowed` then write.
- Deeper: `new = depth[pin_slc] + 1`; same.
- SetDepth: `new = in.digit`; same.
- Pan*: `pan_x += 8` or `-= 8` (right/left), `pan_y += 8` / `-= 8`, then `clamp_pan` using contour bounds: `min_x = 0`, `max_x = inch_to_px(format.last_pin_inch + 2 * format.elbow_inch, pitch)`, `min_y = 0`, `max_y = inch_to_px(format.uncut_depth_inch, pitch) + ORIGIN_Y`.
- Back: no model change (shell handles screen).

After any depth or pin change, call `autopan_to_pin`.

- [ ] **Step 1: Write failing host test for model input**

```cpp
#include "model.h"

void test_model_digit_respects_macs(void) {
    KeyCopierModel m;
    model_init(&m, 0);
    MeasureInput in;
    in.action = MeasureAction_SetDepth;
    in.digit = 6;
    model_apply_measure_input(&m, in);
    TEST_ASSERT_EQUAL_UINT8(1, m.depth[0]);
    in.digit = 5;
    model_apply_measure_input(&m, in);
    TEST_ASSERT_EQUAL_UINT8(5, m.depth[0]);
}

void test_model_pin_walk(void) {
    KeyCopierModel m;
    model_init(&m, 0);
    MeasureInput in;
    in.action = MeasureAction_PinNext;
    in.digit = 0;
    model_apply_measure_input(&m, in);
    TEST_ASSERT_EQUAL_INT(1, m.pin_slc);
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `pio test -e native`

Expected: FAIL compile.

- [ ] **Step 3: Implement `model.cpp` + `measure_view.cpp`**

`measure_view.cpp` (`#ifdef ARDUINO`):

```cpp
#include "measure_view.h"
#include <M5Cardputer.h>

void measure_view_draw(KeyCopierModel* m) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    Segment segs[256];
    int n = build_contour(m->format, m->depth, segs, 256);
    for (int i = 0; i < n; i++) {
        Pixel a = world_to_screen(segs[i].x0, segs[i].y0, m->inches_per_px, ORIGIN_X, ORIGIN_Y,
                                  m->pan_x, m->pan_y);
        Pixel b = world_to_screen(segs[i].x1, segs[i].y1, m->inches_per_px, ORIGIN_X, ORIGIN_Y,
                                  m->pan_x, m->pan_y);
        M5Cardputer.Display.drawLine(a.x, a.y, b.x, b.y, TFT_WHITE);
    }
    for (int p = 1; p <= m->format.pin_num; p++) {
        Pixel c = world_to_screen(pin_center_inch(m->format, p), 0.0, m->inches_per_px, ORIGIN_X,
                                  ORIGIN_Y, m->pan_x, m->pan_y);
        char digit[2] = {(char)('0' + m->depth[p - 1]), 0};
        M5Cardputer.Display.setTextColor((p - 1 == m->pin_slc) ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.drawString(digit, c.x - 3, c.y - 12);
    }
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.drawString(m->format.format_name, 180, 2);
}

void measure_view_handle_char(KeyCopierModel* m, char c) {
    model_apply_measure_input(m, measure_input_from_char(c));
}
```

HUD must not change `ORIGIN_*` or segment endpoints.

- [ ] **Step 4: Run host tests**

Run: `pio test -e native`

Expected: model tests PASS.

- [ ] **Step 5: Commit**

```bash
git add src/model.h src/model.cpp src/measure_view.h src/measure_view.cpp platformio.ini test/test_native/test_native.cpp
git commit -m "feat: measure overlay view and typed pin depths"
```

---

### Task 8: App shell, SD, settings, help

**Files:**
- Create: `src/app_shell.h`
- Create: `src/app_shell.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- Consumes: all previous modules, M5Cardputer, SD, Preferences (NVS)
- Produces: `void app_setup();` `void app_loop();`

Screen enum:

```cpp
typedef enum {
    Screen_Menu,
    Screen_Mfr,
    Screen_Format,
    Screen_Measure,
    Screen_Save,
    Screen_Load,
    Screen_Settings,
    Screen_Help,
    Screen_Alert
} Screen;
```

`app_setup`:

1. `M5Cardputer.begin(M5.config(), true);` rotation 1; backlight on.
2. Try SD. Cardputer SD pins from M5 docs: `CS=G12`, `MOSI=G14`, `SCK=G40`, `MISO=G39`. Use `SPI.begin(40, 39, 14, 12);` then `SD.begin(12, SPI, 25000000)`. If mount works, `SD.mkdir("/adv-keycopy")` and also try `SD.mkdir("/sd/adv-keycopy")` only if `/sd` exists. Set `g_sd_ok`. Prefer listing whichever directory actually exists; store the chosen prefix in `g_keycopy_dir` (`"/adv-keycopy"` or `"/sd/adv-keycopy"`).
3. Load settings: if SD, read `g_keycopy_dir + "/settings.ini"` into a buffer and `parse_settings_ini`. Else `Preferences prefs; prefs.begin("keycopy", true);` get `double pitch` key `"pitch"`. If missing or `<= 0`, `DEFAULT_INCHES_PER_PX`.
4. `model_init(&g_model, 0);` then `g_model.inches_per_px = pitch`.
5. `g_screen = Screen_Menu`.

`app_loop`: `M5Cardputer.update();` on keyboard change, for each `status.word` char, dispatch. `status.enter` is Enter. ESC: many Cardputer firmwares send `` ` ``; treat that as back.

**Menu** items (cursor `g_menu_i`):

0 Select Key Format → `Screen_Mfr`  
1 Measure → `Screen_Measure`  
2 Save → if `!g_sd_ok` show alert `NO SD` then menu; else `Screen_Save`  
3 Load → same `NO SD` guard; else scan dir for `*.keycopy` (max 32 names, 32 chars each)  
4 Settings → `Screen_Settings`  
5 Help → `Screen_Help`

`;` / `,` move up; `/` / `.` move down; Enter select; `` ` `` ignored on menu (or no-op).

**Manufacturer list:** unique `all_formats[i].manufacturer` in table order. Enter → format list filtered by that string. `` ` `` → menu.

**Format list:** Enter → `model_init(&g_model, format_index);` keep `inches_per_px`; `g_screen = Screen_Measure`.

**Measure:** `measure_view_draw` each frame or on input. Chars go to `measure_view_handle_char`. `` ` `` → menu. Also map HID arrows to the same actions if `Keyboard` exposes them; character map is required.

**Save:** show `Name:` plus `g_model.name` while typing. Backspace via `status.del`. Enter: if `!keycopy_name_ok` show `BAD NAME` stay on Save; else `keycopy_serialize` into a 512-byte buffer, write `g_keycopy_dir + "/" + name + ".keycopy"`. On write fail show `WRITE FAIL` stay on Save; on success → menu.

**Load:** list files; Enter read whole file (max 1024 bytes) → `keycopy_parse`. On false, alert `BAD FILE` and stay on Load (model unchanged). On true, `model_init` is **not** enough — set `format_index`, copy `all_formats[idx]`, copy depths, `data_loaded = true`, keep pitch, reset pan, go Measure.

**Settings:** show current pitch with 6 decimals. `-` subtract `0.000010`, `=` add `0.000010`, reject if result `<= 0`. Enter or `` ` `` saves: if SD write `settings.ini` via `format_settings_ini`; always write NVS `"pitch"`. Then menu.

**Help** (scroll with `;` `/`):

```
Place key on the glass.
Align the shoulder.
Type 0-9 to set a pin.
, . select pin
- = [ ] pan
One eye closed.
github.com/zinongli/KeyCopier
```

`` ` `` → menu.

**Alert:** draw the message 1.2s or until key, then `g_screen = g_alert_return`.

`main.cpp`:

```cpp
#ifdef ARDUINO
#include "app_shell.h"
void setup() { app_setup(); }
void loop() { app_loop(); }
#endif
```

No QR view. No Wi-Fi.

- [ ] **Step 1: Implement `app_shell` and switch `main.cpp`**

Write the files as specified above. Keep functions small: `draw_menu`, `handle_menu_char`, `try_mount_sd`, `load_pitch`, `save_pitch`.

- [ ] **Step 2: Build device firmware**

Run: `pio run -e cardputer-adv`

Expected: SUCCESS compile/link.

- [ ] **Step 3: Re-run host tests**

Run: `pio test -e native`

Expected: all previous tests still PASS (device files must stay out of the native `build_src_filter`).

- [ ] **Step 4: On-device smoke (when hardware is available)**

1. Download mode: power OFF, hold G0, power ON, `pio run -e cardputer-adv -t upload`
2. KW1: lay a real KW1 (or any KW blank) on the glass; type a depth; arrow-nudge
3. Switch to SC4 or B102; pan with `-` `=` until the selected pin is on screen and 1:1
4. Save `test1` → confirm `adv-keycopy/test1.keycopy` on the card in a computer
5. Change a pin, Load `test1`, confirm depths restore
6. Settings: bump pitch, confirm overlay moves, reboot, confirm pitch sticks
7. Remove SD, Save → `NO SD`

- [ ] **Step 5: Confirm `.gitignore` still holds**

```bash
git status
```

Expected: no `.pio/`, no `*.elf` / `*.bin` unless you force-add. Only source and docs.

- [ ] **Step 6: Commit**

```bash
git add src/app_shell.h src/app_shell.cpp src/main.cpp
git commit -m "feat: Cardputer menus, SD keycopy, and settings"
```

---

### Task 9: README check and final host suite

**Files:**
- Modify: `README.md` only if flash flags, paths, or keys differ from what shipped
- Modify: `test/test_native/test_native.cpp` if any spec case is still missing

**Interfaces:**
- Consumes: shipped behavior
- Produces: README matches the binary; `pio test -e native` covers every host case in the spec

- [ ] **Step 1: Grep the spec test list against `test_native.cpp`**

Spec requires: KW1/SC4 pin centers, shoulder, elbow; H75 bottom edge; MACS accept/reject; `.keycopy` round-trip; reject unknown format; reject wrong-length bitting.

If any `RUN_TEST` is missing, add it.

- [ ] **Step 2: Run the full native suite**

Run: `pio test -e native`

Expected: all tests PASS.

- [ ] **Step 3: Align README**

If the SD path that actually worked is `/adv-keycopy` (no `/sd` prefix), keep the README sentence that both are the same folder on the card. If a key binding had to change because the Adv does not emit that character, update the keyboard tables in `README.md` and `docs/superpowers/specs/2026-08-30-adv-key-copier-design.md` to the keys you actually used.

- [ ] **Step 4: Final `git status` vs `.gitignore`**

```bash
git status
```

Expected: clean or only intentional source/docs. Never `.pio/`.

- [ ] **Step 5: Commit if README or tests changed**

```bash
git add README.md test/test_native/test_native.cpp docs/superpowers/specs/2026-08-30-adv-key-copier-design.md
git commit -m "docs: match README and host tests to shipped Adv app"
```

Skip this commit if nothing changed.

---

## Self-review (plan vs spec)

| Spec item | Task |
| --- | --- |
| PlatformIO + Arduino + M5Cardputer | 1, 8 |
| 23 formats, sides/stop explicit | 2 |
| `depth_change_allowed` MACS | 3 |
| Inch contour, shoulder, elbow, double-sided | 4 |
| Flipper `.keycopy` + `/adv-keycopy` + name rules | 5, 8 |
| `settings.ini` / NVS / `0.004140` | 5, 8 |
| 1:1 + pan + auto-pan | 6, 7 |
| Type-to-set + arrows | 6, 7 |
| Menus, help, no QR, `NO SD` | 8 |
| Host tests listed in spec | 2–6, 9 |
| On-device smoke | 8 |
| `.gitignore` | 1, 8, 9 |
| Client README | already written; 9 syncs |

No TBD steps. `build_contour` is specified as a Flipper-inch port with a complete loop. Native env never compiles `main.cpp` / `app_shell.cpp` / `measure_view.cpp`.
