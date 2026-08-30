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
