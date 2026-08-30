#ifndef VIEW_MATH_H
#define VIEW_MATH_H

#include "keycopy_io.h"
#include <stdint.h>

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

#endif
