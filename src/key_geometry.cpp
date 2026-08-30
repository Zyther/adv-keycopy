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
