#include "view_math.h"

#include <math.h>

int inch_to_px(double inch, double inches_per_px) {
    if (inches_per_px <= 0.0) inches_per_px = DEFAULT_INCHES_PER_PX;
    return (int)lround(inch / inches_per_px);
}

Pixel world_to_screen(double inch_x, double inch_y, double inches_per_px, int origin_x,
                      int origin_y, int pan_x, int pan_y) {
    Pixel p;
    p.x = origin_x + inch_to_px(inch_x, inches_per_px) - pan_x;
    p.y = origin_y + inch_to_px(inch_y, inches_per_px) - pan_y;
    return p;
}

static int clamp_axis(int pan, int minv, int maxv, int view) {
    int span = maxv - minv;
    if (span <= view) return minv;
    int max_pan = maxv - view;
    if (pan < minv) return minv;
    if (pan > max_pan) return max_pan;
    return pan;
}

void clamp_pan(int* pan_x, int* pan_y, int min_x, int max_x, int min_y, int max_y, int view_w,
               int view_h) {
    *pan_x = clamp_axis(*pan_x, min_x, max_x, view_w);
    *pan_y = clamp_axis(*pan_y, min_y, max_y, view_h);
}

void autopan_to_pin(int pin_px_x, int pin_px_y, int* pan_x, int* pan_y, int view_w, int view_h,
                    int margin) {
    int sx = pin_px_x - *pan_x;
    if (sx < margin) *pan_x = pin_px_x - margin;
    if (sx > view_w - margin) *pan_x = pin_px_x - (view_w - margin);
    int sy = pin_px_y - *pan_y;
    if (sy < margin) *pan_y = pin_px_y - margin;
    if (sy > view_h - margin) *pan_y = pin_px_y - (view_h - margin);
}

MeasureInput measure_input_from_char(char c) {
    MeasureInput out = {MeasureAction_None, 0};
    switch (c) {
        case ',':
            out.action = MeasureAction_PinPrev;
            break;
        case '/':
            out.action = MeasureAction_PinNext;
            break;
        case ';':
            out.action = MeasureAction_Shallower;
            break;
        case '.':
            out.action = MeasureAction_Deeper;
            break;
        case '-':
            out.action = MeasureAction_PanLeft;
            break;
        case '=':
            out.action = MeasureAction_PanRight;
            break;
        case '[':
            out.action = MeasureAction_PanUp;
            break;
        case ']':
            out.action = MeasureAction_PanDown;
            break;
        case '`':
        case 27:
            out.action = MeasureAction_Back;
            break;
        default:
            if (c >= '0' && c <= '9') {
                out.action = MeasureAction_SetDepth;
                out.digit = (uint8_t)(c - '0');
            }
            break;
    }
    return out;
}
