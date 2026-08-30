#include "key_geometry.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void add_seg(Segment* out, int cap, int* n, double x0, double y0, double x1, double y1) {
    if (*n < 0 || *n >= cap) return;
    if (x0 == x1 && y0 == y1) return;
    out[*n].x0 = x0;
    out[*n].y0 = y0;
    out[*n].x1 = x1;
    out[*n].y1 = y1;
    (*n)++;
}

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
