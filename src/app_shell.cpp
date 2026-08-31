#ifdef ARDUINO
#include "app_shell.h"
#include "keycopy_io.h"
#include "key_formats.h"
#include "measure_view.h"
#include "model.h"

#include <M5Cardputer.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <stdio.h>
#include <string.h>

#define HID_ARROW_RIGHT 0x4F
#define HID_ARROW_LEFT 0x50
#define HID_ARROW_DOWN 0x51
#define HID_ARROW_UP 0x52

#define ALERT_MS 1200
#define PITCH_STEP 0.000010
#define LOAD_MAX 32
#define LINE_H 14
#define LIST_TOP 18
#define LIST_VIS 8

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

static const char* k_menu[] = {
    "Select Key Format",
    "Measure",
    "Save",
    "Load",
    "Settings",
    "Help",
};
static const int k_menu_n = 6;

static const char* k_help[] = {
    "Place key on the glass.",
    "Align the shoulder.",
    "Type 0-9 to set a pin.",
    ", . select pin",
    "- = [ ] pan",
    "One eye closed.",
    "github.com/Zyther/adv-keycopy",
};
static const int k_help_n = 7;

static Screen g_screen;
static Screen g_alert_return;
static KeyCopierModel g_model;
static bool g_sd_ok;
static const char* g_keycopy_dir;
static int g_menu_i;
static int g_mfr_i;
static int g_fmt_i;
static int g_load_i;
static int g_help_top;
static char g_alert_msg[24];
static uint32_t g_alert_at;

static const char* g_mfrs[FORMAT_NUM];
static int g_mfr_n;
static int g_fmt_idx[FORMAT_NUM];
static int g_fmt_n;
static char g_load_names[LOAD_MAX][32];
static int g_load_n;
static bool g_dirty;

static bool nav_up(char c) {
    return c == ';' || c == ',';
}

static bool nav_down(char c) {
    return c == '/' || c == '.';
}

static int clamp_i(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void show_alert(const char* msg, Screen ret) {
    strncpy(g_alert_msg, msg, sizeof(g_alert_msg) - 1);
    g_alert_msg[sizeof(g_alert_msg) - 1] = 0;
    g_alert_return = ret;
    g_alert_at = millis();
    g_screen = Screen_Alert;
    g_dirty = true;
}

static bool sd_dir_exists(const char* path) {
    File f = SD.open(path);
    if (!f) return false;
    bool ok = f.isDirectory();
    f.close();
    return ok;
}

static void try_mount_sd() {
    g_keycopy_dir = "/adv-keycopy";
    SPI.begin(40, 39, 14, 12);
    if (!SD.begin(12, SPI, 25000000)) {
        g_sd_ok = false;
        return;
    }
    g_sd_ok = true;
    SD.mkdir("/adv-keycopy");
    if (sd_dir_exists("/sd")) {
        SD.mkdir("/sd/adv-keycopy");
    }
    if (sd_dir_exists("/adv-keycopy")) {
        g_keycopy_dir = "/adv-keycopy";
    } else if (sd_dir_exists("/sd/adv-keycopy")) {
        g_keycopy_dir = "/sd/adv-keycopy";
    }
}

static double load_pitch() {
    if (g_sd_ok) {
        char path[64];
        snprintf(path, sizeof(path), "%s/settings.ini", g_keycopy_dir);
        File f = SD.open(path, FILE_READ);
        if (f) {
            char buf[64] = {0};
            int n = f.read((uint8_t*)buf, sizeof(buf) - 1);
            f.close();
            double pitch = 0.0;
            if (n > 0 && parse_settings_ini(buf, &pitch) && pitch > 0.0) {
                return pitch;
            }
        }
        return DEFAULT_INCHES_PER_PX;
    }
    Preferences prefs;
    prefs.begin("keycopy", true);
    double pitch = prefs.getDouble("pitch", 0.0);
    prefs.end();
    if (pitch <= 0.0) return DEFAULT_INCHES_PER_PX;
    return pitch;
}

static void save_pitch(double pitch) {
    if (g_sd_ok) {
        char path[64];
        char buf[64];
        snprintf(path, sizeof(path), "%s/settings.ini", g_keycopy_dir);
        int n = format_settings_ini(pitch, buf, sizeof(buf));
        if (n > 0) {
            File f = SD.open(path, FILE_WRITE);
            if (f) {
                f.write((const uint8_t*)buf, (size_t)n);
                f.close();
            }
        }
    }
    Preferences prefs;
    prefs.begin("keycopy", false);
    prefs.putDouble("pitch", pitch);
    prefs.end();
}

static void build_mfr_list() {
    g_mfr_n = 0;
    for (int i = 0; i < FORMAT_NUM; i++) {
        const char* m = all_formats[i].manufacturer;
        bool seen = false;
        for (int j = 0; j < g_mfr_n; j++) {
            if (strcmp(g_mfrs[j], m) == 0) {
                seen = true;
                break;
            }
        }
        if (!seen) g_mfrs[g_mfr_n++] = m;
    }
}

static void build_fmt_list(const char* mfr) {
    g_fmt_n = 0;
    for (int i = 0; i < FORMAT_NUM; i++) {
        if (strcmp(all_formats[i].manufacturer, mfr) == 0) {
            g_fmt_idx[g_fmt_n++] = i;
        }
    }
}

static const char* file_basename(const char* path) {
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static bool ends_with_keycopy(const char* name) {
    size_t n = strlen(name);
    return n > 8 && strcmp(name + n - 8, ".keycopy") == 0;
}

static void scan_keycopy_dir() {
    g_load_n = 0;
    if (!g_sd_ok) return;
    File dir = SD.open(g_keycopy_dir);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return;
    }
    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory() && g_load_n < LOAD_MAX) {
            const char* base = file_basename(f.name());
            if (ends_with_keycopy(base)) {
                size_t stem = strlen(base) - 8;
                if (stem > 0 && stem < 32) {
                    memcpy(g_load_names[g_load_n], base, stem);
                    g_load_names[g_load_n][stem] = 0;
                    g_load_n++;
                }
            }
        }
        f.close();
        f = dir.openNextFile();
    }
    dir.close();
}

static void apply_loaded_key(int format_index, const uint8_t* depths, const char* name) {
    double pitch = g_model.inches_per_px;
    g_model.format_index = format_index;
    g_model.format = all_formats[format_index];
    for (int i = 0; i < MODEL_MAX_PINS; i++) {
        g_model.depth[i] = (uint8_t)g_model.format.min_depth_ind;
    }
    int n = g_model.format.pin_num;
    if (n > MODEL_MAX_PINS) n = MODEL_MAX_PINS;
    memcpy(g_model.depth, depths, (size_t)n);
    g_model.data_loaded = true;
    g_model.inches_per_px = pitch;
    g_model.pan_x = 0;
    g_model.pan_y = 0;
    g_model.pin_slc = 0;
    strncpy(g_model.name, name, MODEL_NAME_LEN - 1);
    g_model.name[MODEL_NAME_LEN - 1] = 0;
}

static bool write_keycopy() {
    char ser[512];
    if (!keycopy_serialize(g_model.format, g_model.depth, ser, sizeof(ser))) {
        return false;
    }
    char path[96];
    snprintf(path, sizeof(path), "%s/%s.keycopy", g_keycopy_dir, g_model.name);
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    size_t len = strlen(ser);
    size_t wrote = f.write((const uint8_t*)ser, len);
    f.close();
    return wrote == len;
}

static bool read_keycopy(const char* name, char* buf, size_t buf_len) {
    char path[96];
    snprintf(path, sizeof(path), "%s/%s.keycopy", g_keycopy_dir, name);
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    size_t n = f.read((uint8_t*)buf, buf_len - 1);
    f.close();
    buf[n] = 0;
    return n > 0;
}

static void draw_title(const char* title) {
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.drawString(title, 4, 2);
}

static void draw_list_item(int vis_i, int cursor, int index, const char* text) {
    int y = LIST_TOP + vis_i * LINE_H;
    M5Cardputer.Display.setTextColor((index == cursor) ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.drawString(text, 4, y);
}

static int list_offset(int cursor, int count) {
    if (count <= LIST_VIS) return 0;
    int off = cursor - (LIST_VIS / 2);
    if (off < 0) off = 0;
    if (off > count - LIST_VIS) off = count - LIST_VIS;
    return off;
}

static void draw_menu() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    draw_title("Key Copier");
    int off = list_offset(g_menu_i, k_menu_n);
    for (int v = 0; v < LIST_VIS && off + v < k_menu_n; v++) {
        draw_list_item(v, g_menu_i, off + v, k_menu[off + v]);
    }
}

static void draw_mfr() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    draw_title("Manufacturer");
    int off = list_offset(g_mfr_i, g_mfr_n);
    for (int v = 0; v < LIST_VIS && off + v < g_mfr_n; v++) {
        draw_list_item(v, g_mfr_i, off + v, g_mfrs[off + v]);
    }
}

static void draw_format() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    draw_title("Format");
    int off = list_offset(g_fmt_i, g_fmt_n);
    for (int v = 0; v < LIST_VIS && off + v < g_fmt_n; v++) {
        int idx = g_fmt_idx[off + v];
        draw_list_item(v, g_fmt_i, off + v, all_formats[idx].format_name);
    }
}

static void draw_save() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    draw_title("Save");
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    char line[48];
    snprintf(line, sizeof(line), "Name: %s", g_model.name);
    M5Cardputer.Display.drawString(line, 4, LIST_TOP);
}

static void draw_load() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    draw_title("Load");
    if (g_load_n == 0) {
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.drawString("(empty)", 4, LIST_TOP);
        return;
    }
    int off = list_offset(g_load_i, g_load_n);
    for (int v = 0; v < LIST_VIS && off + v < g_load_n; v++) {
        draw_list_item(v, g_load_i, off + v, g_load_names[off + v]);
    }
}

static void draw_settings() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    draw_title("Settings");
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    char line[40];
    snprintf(line, sizeof(line), "%.6f", g_model.inches_per_px);
    M5Cardputer.Display.drawString(line, 4, LIST_TOP);
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5Cardputer.Display.drawString("- =  Enter save", 4, LIST_TOP + LINE_H);
}

static void draw_help() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    draw_title("Help");
    int max_top = (k_help_n > LIST_VIS) ? k_help_n - LIST_VIS : 0;
    g_help_top = clamp_i(g_help_top, 0, max_top);
    for (int v = 0; v < LIST_VIS && g_help_top + v < k_help_n; v++) {
        M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5Cardputer.Display.drawString(k_help[g_help_top + v], 4, LIST_TOP + v * LINE_H);
    }
}

static void draw_alert() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5Cardputer.Display.drawString(g_alert_msg, 4, 56);
}

static void draw_screen() {
    switch (g_screen) {
        case Screen_Menu:
            draw_menu();
            break;
        case Screen_Mfr:
            draw_mfr();
            break;
        case Screen_Format:
            draw_format();
            break;
        case Screen_Measure:
            measure_view_draw(&g_model);
            break;
        case Screen_Save:
            draw_save();
            break;
        case Screen_Load:
            draw_load();
            break;
        case Screen_Settings:
            draw_settings();
            break;
        case Screen_Help:
            draw_help();
            break;
        case Screen_Alert:
            draw_alert();
            break;
    }
}

static void select_format() {
    double pitch = g_model.inches_per_px;
    model_init(&g_model, g_fmt_idx[g_fmt_i]);
    g_model.inches_per_px = pitch;
    g_screen = Screen_Measure;
}

static void handle_menu_char(char c) {
    if (nav_up(c)) {
        g_menu_i = clamp_i(g_menu_i - 1, 0, k_menu_n - 1);
    } else if (nav_down(c)) {
        g_menu_i = clamp_i(g_menu_i + 1, 0, k_menu_n - 1);
    }
}

static void handle_menu_enter() {
    switch (g_menu_i) {
        case 0:
            g_mfr_i = 0;
            g_screen = Screen_Mfr;
            break;
        case 1:
            g_screen = Screen_Measure;
            break;
        case 2:
            if (!g_sd_ok) {
                show_alert("NO SD", Screen_Menu);
            } else {
                g_screen = Screen_Save;
            }
            break;
        case 3:
            if (!g_sd_ok) {
                show_alert("NO SD", Screen_Menu);
            } else {
                scan_keycopy_dir();
                g_load_i = 0;
                g_screen = Screen_Load;
            }
            break;
        case 4:
            g_screen = Screen_Settings;
            break;
        case 5:
            g_help_top = 0;
            g_screen = Screen_Help;
            break;
        default:
            break;
    }
}

static void handle_mfr_char(char c) {
    if (c == '`') {
        g_screen = Screen_Menu;
        return;
    }
    if (nav_up(c)) {
        g_mfr_i = clamp_i(g_mfr_i - 1, 0, g_mfr_n - 1);
    } else if (nav_down(c)) {
        g_mfr_i = clamp_i(g_mfr_i + 1, 0, g_mfr_n - 1);
    }
}

static void handle_mfr_enter() {
    build_fmt_list(g_mfrs[g_mfr_i]);
    g_fmt_i = 0;
    g_screen = Screen_Format;
}

static void handle_format_char(char c) {
    if (c == '`') {
        g_screen = Screen_Mfr;
        return;
    }
    if (nav_up(c)) {
        g_fmt_i = clamp_i(g_fmt_i - 1, 0, g_fmt_n - 1);
    } else if (nav_down(c)) {
        g_fmt_i = clamp_i(g_fmt_i + 1, 0, g_fmt_n - 1);
    }
}

static void handle_measure_char(char c) {
    if (c == '`') {
        g_screen = Screen_Menu;
        return;
    }
    measure_view_handle_char(&g_model, c);
}

static void handle_measure_hid(const Keyboard_Class::KeysState& status) {
    for (auto hid : status.hid_keys) {
        char mapped = 0;
        switch (hid) {
            case HID_ARROW_LEFT:
                mapped = ',';
                break;
            case HID_ARROW_RIGHT:
                mapped = '.';
                break;
            case HID_ARROW_UP:
                mapped = ';';
                break;
            case HID_ARROW_DOWN:
                mapped = '/';
                break;
            default:
                break;
        }
        if (mapped) measure_view_handle_char(&g_model, mapped);
    }
}

static void handle_save_char(char c) {
    if (c == '`') {
        g_screen = Screen_Menu;
        return;
    }
    size_t n = strlen(g_model.name);
    if (n + 1 >= MODEL_NAME_LEN) return;
    if (c < 32 || c > 126) return;
    g_model.name[n] = c;
    g_model.name[n + 1] = 0;
}

static void handle_save_del() {
    size_t n = strlen(g_model.name);
    if (n == 0) return;
    g_model.name[n - 1] = 0;
}

static void handle_save_enter() {
    if (!keycopy_name_ok(g_model.name)) {
        show_alert("BAD NAME", Screen_Save);
        return;
    }
    if (!write_keycopy()) {
        show_alert("WRITE FAIL", Screen_Save);
        return;
    }
    g_screen = Screen_Menu;
}

static void handle_load_char(char c) {
    if (c == '`') {
        g_screen = Screen_Menu;
        return;
    }
    if (g_load_n <= 0) return;
    if (nav_up(c)) {
        g_load_i = clamp_i(g_load_i - 1, 0, g_load_n - 1);
    } else if (nav_down(c)) {
        g_load_i = clamp_i(g_load_i + 1, 0, g_load_n - 1);
    }
}

static void handle_load_enter() {
    if (g_load_n <= 0) return;
    static char buf[1024];
    if (!read_keycopy(g_load_names[g_load_i], buf, sizeof(buf))) {
        show_alert("BAD FILE", Screen_Load);
        return;
    }
    int idx = -1;
    uint8_t depths[MODEL_MAX_PINS];
    if (!keycopy_parse(buf, &idx, depths, MODEL_MAX_PINS)) {
        show_alert("BAD FILE", Screen_Load);
        return;
    }
    apply_loaded_key(idx, depths, g_load_names[g_load_i]);
    g_screen = Screen_Measure;
}

static void handle_settings_char(char c) {
    if (c == '`' || c == '\n') {
        save_pitch(g_model.inches_per_px);
        g_screen = Screen_Menu;
        return;
    }
    if (c == '-') {
        double next = g_model.inches_per_px - PITCH_STEP;
        if (next > 0.0) g_model.inches_per_px = next;
    } else if (c == '=') {
        double next = g_model.inches_per_px + PITCH_STEP;
        if (next > 0.0) g_model.inches_per_px = next;
    }
}

static void handle_help_char(char c) {
    if (c == '`') {
        g_screen = Screen_Menu;
        return;
    }
    int max_top = (k_help_n > LIST_VIS) ? k_help_n - LIST_VIS : 0;
    if (c == ';') {
        g_help_top = clamp_i(g_help_top - 1, 0, max_top);
    } else if (c == '/') {
        g_help_top = clamp_i(g_help_top + 1, 0, max_top);
    }
}

static void dispatch(const Keyboard_Class::KeysState& status) {
    if (g_screen == Screen_Measure) {
        handle_measure_hid(status);
    }
    if (status.enter) {
        switch (g_screen) {
            case Screen_Menu:
                handle_menu_enter();
                break;
            case Screen_Mfr:
                handle_mfr_enter();
                break;
            case Screen_Format:
                select_format();
                break;
            case Screen_Save:
                handle_save_enter();
                break;
            case Screen_Load:
                handle_load_enter();
                break;
            case Screen_Settings:
                save_pitch(g_model.inches_per_px);
                g_screen = Screen_Menu;
                break;
            default:
                break;
        }
    }
    if ((status.backspace || status.del) && g_screen == Screen_Save) {
        handle_save_del();
    }
    if (status.esc) {
        switch (g_screen) {
            case Screen_Mfr:
                handle_mfr_char('`');
                break;
            case Screen_Format:
                handle_format_char('`');
                break;
            case Screen_Measure:
                handle_measure_char('`');
                break;
            case Screen_Save:
                handle_save_char('`');
                break;
            case Screen_Load:
                handle_load_char('`');
                break;
            case Screen_Settings:
                handle_settings_char('`');
                break;
            case Screen_Help:
                handle_help_char('`');
                break;
            default:
                break;
        }
    }
    for (auto c : status.word) {
        switch (g_screen) {
            case Screen_Menu:
                handle_menu_char(c);
                break;
            case Screen_Mfr:
                handle_mfr_char(c);
                break;
            case Screen_Format:
                handle_format_char(c);
                break;
            case Screen_Measure:
                handle_measure_char(c);
                break;
            case Screen_Save:
                handle_save_char(c);
                break;
            case Screen_Load:
                handle_load_char(c);
                break;
            case Screen_Settings:
                handle_settings_char(c);
                break;
            case Screen_Help:
                handle_help_char(c);
                break;
            default:
                break;
        }
    }
}

void app_setup() {
    M5Cardputer.begin(M5.config(), true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(255);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    try_mount_sd();
    double pitch = load_pitch();
    model_init(&g_model, 0);
    g_model.inches_per_px = pitch;
    build_mfr_list();
    g_menu_i = 0;
    g_screen = Screen_Menu;
    g_dirty = true;
    draw_screen();
    g_dirty = false;
}

void app_loop() {
    M5Cardputer.update();

    bool key_change = M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed();
    Keyboard_Class::KeysState status;
    if (key_change) {
        status = M5Cardputer.Keyboard.keysState();
    }

    if (g_screen == Screen_Alert) {
        if ((millis() - g_alert_at) >= ALERT_MS || key_change) {
            g_screen = g_alert_return;
            g_dirty = true;
        }
        if (g_dirty) {
            draw_screen();
            g_dirty = false;
        }
        return;
    }

    if (key_change) {
        dispatch(status);
        g_dirty = true;
    }
    if (g_dirty) {
        draw_screen();
        g_dirty = false;
    }
}

#endif
