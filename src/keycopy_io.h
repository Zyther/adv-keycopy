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
