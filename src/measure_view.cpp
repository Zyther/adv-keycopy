#ifdef ARDUINO
#include "measure_view.h"
#include "key_geometry.h"
#include <M5Cardputer.h>

void measure_view_draw(KeyCopierModel* m) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    static Segment segs[128];
    int n = build_contour(m->format, m->depth, segs, 128);
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
#endif
