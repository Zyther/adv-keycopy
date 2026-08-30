#ifndef MODEL_H
#define MODEL_H

#include "key_formats.h"
#include "view_math.h"
#include <stdint.h>
#include <stdbool.h>

#define MODEL_MAX_PINS 16
#define MODEL_NAME_LEN 32

typedef struct {
    int format_index;
    KeyFormat format;
    uint8_t depth[MODEL_MAX_PINS];
    int pin_slc;
    int pan_x;
    int pan_y;
    double inches_per_px;
    bool data_loaded;
    char name[MODEL_NAME_LEN];
} KeyCopierModel;

void model_init(KeyCopierModel* m, int format_index);
void model_apply_measure_input(KeyCopierModel* m, MeasureInput in);

#endif
