#include "model.h"
#include "key_geometry.h"

#define AUTOPAN_MARGIN 16

static void model_autopan(KeyCopierModel* m) {
    int pin_px_x = inch_to_px(pin_center_inch(m->format, m->pin_slc + 1), m->inches_per_px);
    int pin_px_y = ORIGIN_Y;
    autopan_to_pin(pin_px_x, pin_px_y, &m->pan_x, &m->pan_y, SCREEN_W, SCREEN_H, AUTOPAN_MARGIN);
}

static void model_clamp_pan(KeyCopierModel* m) {
    int max_x = inch_to_px(m->format.last_pin_inch + 2.0 * m->format.elbow_inch, m->inches_per_px);
    int max_y = inch_to_px(m->format.uncut_depth_inch, m->inches_per_px) + ORIGIN_Y;
    clamp_pan(&m->pan_x, &m->pan_y, 0, max_x, 0, max_y, SCREEN_W, SCREEN_H);
}

static void model_try_set_depth(KeyCopierModel* m, uint8_t new_depth) {
    if (depth_change_allowed(m->format, m->depth, m->pin_slc, new_depth)) {
        m->depth[m->pin_slc] = new_depth;
        model_autopan(m);
    }
}

static void model_clamp_pin(KeyCopierModel* m) {
    if (m->pin_slc < 0) m->pin_slc = 0;
    if (m->pin_slc >= m->format.pin_num) m->pin_slc = m->format.pin_num - 1;
}

void model_init(KeyCopierModel* m, int format_index) {
    m->format_index = format_index;
    m->format = all_formats[format_index];
    for (int i = 0; i < MODEL_MAX_PINS; i++) {
        m->depth[i] = (uint8_t)m->format.min_depth_ind;
    }
    m->pin_slc = 0;
    m->pan_x = 0;
    m->pan_y = 0;
    m->inches_per_px = DEFAULT_INCHES_PER_PX;
    m->data_loaded = false;
    m->name[0] = 0;
}

void model_apply_measure_input(KeyCopierModel* m, MeasureInput in) {
    switch (in.action) {
        case MeasureAction_PinPrev:
            m->pin_slc -= 1;
            model_clamp_pin(m);
            model_autopan(m);
            break;
        case MeasureAction_PinNext:
            m->pin_slc += 1;
            model_clamp_pin(m);
            model_autopan(m);
            break;
        case MeasureAction_Shallower:
            model_try_set_depth(m, (uint8_t)(m->depth[m->pin_slc] - 1));
            break;
        case MeasureAction_Deeper:
            model_try_set_depth(m, (uint8_t)(m->depth[m->pin_slc] + 1));
            break;
        case MeasureAction_SetDepth:
            model_try_set_depth(m, in.digit);
            break;
        case MeasureAction_PanLeft:
            m->pan_x -= 8;
            model_clamp_pan(m);
            break;
        case MeasureAction_PanRight:
            m->pan_x += 8;
            model_clamp_pan(m);
            break;
        case MeasureAction_PanUp:
            m->pan_y -= 8;
            model_clamp_pan(m);
            break;
        case MeasureAction_PanDown:
            m->pan_y += 8;
            model_clamp_pan(m);
            break;
        case MeasureAction_Back:
        case MeasureAction_None:
        default:
            break;
    }
}
