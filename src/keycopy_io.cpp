#include "keycopy_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool name_has_bad_char(const char* name) {
    static const char bad[] = "/\\:*?\"<>|";
    for (const char* p = name; *p; p++) {
        if (strchr(bad, *p)) return true;
    }
    return false;
}

bool keycopy_name_ok(const char* name) {
    if (!name || name[0] == '\0') return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    return !name_has_bad_char(name);
}

bool keycopy_serialize(const KeyFormat& format, const uint8_t* depths, char* buf, size_t buf_len) {
    if (!depths || !buf || buf_len == 0) return false;

    char bitting[128];
    int pos = 0;
    for (int i = 0; i < format.pin_num; i++) {
        if (i > 0) {
            if (pos + 1 >= (int)sizeof(bitting)) return false;
            bitting[pos++] = '-';
        }
        int n = snprintf(bitting + pos, sizeof(bitting) - (size_t)pos, "%u", depths[i]);
        if (n < 0 || pos + n >= (int)sizeof(bitting)) return false;
        pos += n;
    }

    int written = snprintf(
        buf, buf_len,
        "Filetype: Flipper Key Copier File\n"
        "Version: 1\n"
        "Manufacturer: %s\n"
        "Format Name: %s\n"
        "Data Sheet: %s\n"
        "Number of Pins: %d\n"
        "Maximum Adjacent Cut Specification (MACS): %d\n"
        "Bitting Pattern: %s\n",
        format.manufacturer,
        format.format_name,
        format.format_link,
        format.pin_num,
        format.macs,
        bitting);
    return written >= 0 && (size_t)written < buf_len;
}

static const char* line_value(const char* line, const char* prefix) {
    size_t n = strlen(prefix);
    if (strncmp(line, prefix, n) != 0) return NULL;
    const char* v = line + n;
    if (*v == ' ') v++;
    return v;
}

static void strip_cr(char* s) {
    size_t n = strlen(s);
    if (n > 0 && s[n - 1] == '\r') s[n - 1] = '\0';
}

static bool parse_bitting(const char* pattern, int pin_num, uint8_t* local_depths) {
    if (!pattern || pin_num <= 0) return false;

    const char* p = pattern;
    int count = 0;
    while (*p) {
        if (count >= pin_num) return false;
        if (*p < '0' || *p > '9') return false;

        unsigned val = 0;
        while (*p >= '0' && *p <= '9') {
            val = val * 10u + (unsigned)(*p - '0');
            if (val > 255u) return false;
            p++;
        }
        local_depths[count++] = (uint8_t)val;

        if (*p == '-') {
            p++;
            if (*p == '\0') return false;
        } else if (*p != '\0') {
            return false;
        }
    }
    return count == pin_num;
}

bool keycopy_parse(const char* text, int* format_index, uint8_t* depths, int depths_cap) {
    if (!text || !format_index || !depths) return false;

    char format_name[64] = {0};
    char bitting[128] = {0};
    bool have_format = false;
    bool have_bitting = false;

    const char* cur = text;
    while (*cur) {
        const char* eol = strchr(cur, '\n');
        size_t line_len = eol ? (size_t)(eol - cur) : strlen(cur);
        char line[256];
        if (line_len >= sizeof(line)) return false;
        memcpy(line, cur, line_len);
        line[line_len] = '\0';
        strip_cr(line);

        const char* v = line_value(line, "Format Name:");
        if (v) {
            strncpy(format_name, v, sizeof(format_name) - 1);
            have_format = true;
        }
        v = line_value(line, "Bitting Pattern:");
        if (v) {
            strncpy(bitting, v, sizeof(bitting) - 1);
            have_bitting = true;
        }

        if (!eol) break;
        cur = eol + 1;
    }

    if (!have_format || !have_bitting) return false;

    int idx = find_format_by_name(format_name);
    if (idx < 0) return false;

    const KeyFormat& fmt = all_formats[idx];
    if (depths_cap < fmt.pin_num) return false;

    uint8_t local_depths[32];
    if (fmt.pin_num > (int)sizeof(local_depths)) return false;
    if (!parse_bitting(bitting, fmt.pin_num, local_depths)) return false;

    *format_index = idx;
    memcpy(depths, local_depths, (size_t)fmt.pin_num);
    return true;
}

bool parse_settings_ini(const char* text, double* out_pitch) {
    if (!text || !out_pitch) return false;
    const char* p = strstr(text, "inches_per_px=");
    if (!p) return false;
    char* end = NULL;
    double pitch = strtod(p + 14, &end);
    if (end == p + 14 || pitch <= 0.0) return false;
    *out_pitch = pitch;
    return true;
}

int format_settings_ini(double pitch, char* buf, size_t buf_len) {
    if (!buf || buf_len == 0) return -1;
    int n = snprintf(buf, buf_len, "inches_per_px=%.6f\n", pitch);
    if (n < 0 || (size_t)n >= buf_len) return -1;
    return n;
}
