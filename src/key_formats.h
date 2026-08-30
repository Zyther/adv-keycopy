#ifndef KEY_FORMATS_H
#define KEY_FORMATS_H

enum { FORMAT_NUM = 23 };

typedef struct {
    const char* manufacturer;
    const char* format_name;
    const char* format_link;
    int sides;
    int stop;
    double first_pin_inch;
    double last_pin_inch;
    double pin_increment_inch;
    int pin_num;
    double pin_width_inch;
    double drill_angle;
    double elbow_inch;
    double uncut_depth_inch;
    double deepest_depth_inch;
    double depth_step_inch;
    int min_depth_ind;
    int max_depth_ind;
    int macs;
    int clearance;
} KeyFormat;

extern const KeyFormat all_formats[FORMAT_NUM];

int find_format_by_name(const char* format_name);

#endif
