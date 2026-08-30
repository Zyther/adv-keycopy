#include <unity.h>
#include <math.h>
#include "key_formats.h"
#include "key_geometry.h"

static void fill_kw1_min(uint8_t* d) {
    for (int i = 0; i < 6; i++) d[i] = 1;
}

void test_native_runner_works(void) {
    TEST_ASSERT_EQUAL_INT(2, 1 + 1);
}

void test_format_count_is_23(void) {
    TEST_ASSERT_EQUAL_INT(23, FORMAT_NUM);
}

void test_kw1_fields(void) {
    TEST_ASSERT_EQUAL_STRING("Kwikset", all_formats[0].manufacturer);
    TEST_ASSERT_EQUAL_STRING("KW1", all_formats[0].format_name);
    TEST_ASSERT_EQUAL_INT(1, all_formats[0].sides);
    TEST_ASSERT_EQUAL_INT(1, all_formats[0].stop);
    TEST_ASSERT_EQUAL_INT(5, all_formats[0].pin_num);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.247, all_formats[0].first_pin_inch);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.847, all_formats[0].last_pin_inch);
    TEST_ASSERT_EQUAL_INT(1, all_formats[0].min_depth_ind);
    TEST_ASSERT_EQUAL_INT(7, all_formats[0].max_depth_ind);
    TEST_ASSERT_EQUAL_INT(4, all_formats[0].macs);
}

void test_sc4_pin_count(void) {
    TEST_ASSERT_EQUAL_STRING("SC4", all_formats[1].format_name);
    TEST_ASSERT_EQUAL_INT(6, all_formats[1].pin_num);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.231, all_formats[1].first_pin_inch);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.012, all_formats[1].last_pin_inch);
}

void test_h75_is_double_sided(void) {
    int i = find_format_by_name("H75");
    TEST_ASSERT_TRUE(i >= 0);
    TEST_ASSERT_EQUAL_INT(2, all_formats[i].sides);
    TEST_ASSERT_EQUAL_INT(2, all_formats[i].stop);
    TEST_ASSERT_EQUAL_INT(8, all_formats[i].pin_num);
}

void test_find_format_unknown(void) {
    TEST_ASSERT_EQUAL_INT(-1, find_format_by_name("NOPE"));
}

void test_macs_rejects_out_of_range(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 0, 0));
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 0, 8));
}

void test_macs_allows_legal_first_pin(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    d[1] = 1;
    TEST_ASSERT_TRUE(depth_change_allowed(all_formats[0], d, 0, 5));
}

void test_macs_rejects_illegal_adjacent(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    d[1] = 1;
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 0, 6));
}

void test_macs_middle_pin_checks_both_neighbors(void) {
    uint8_t d[6];
    fill_kw1_min(d);
    d[0] = 1;
    d[1] = 1;
    d[2] = 1;
    TEST_ASSERT_TRUE(depth_change_allowed(all_formats[0], d, 1, 5));
    TEST_ASSERT_FALSE(depth_change_allowed(all_formats[0], d, 1, 6));
}

static int count_near(const Segment* segs, int n, double x0, double y0, double x1, double y1) {
    int c = 0;
    for (int i = 0; i < n; i++) {
        int match = (fabs(segs[i].x0 - x0) < 0.001 && fabs(segs[i].y0 - y0) < 0.001 &&
                     fabs(segs[i].x1 - x1) < 0.001 && fabs(segs[i].y1 - y1) < 0.001);
        int rev = (fabs(segs[i].x0 - x1) < 0.001 && fabs(segs[i].y0 - y1) < 0.001 &&
                   fabs(segs[i].x1 - x0) < 0.001 && fabs(segs[i].y1 - y0) < 0.001);
        if (match || rev) c++;
    }
    return c;
}

void test_pin_centers_kw1_sc4(void) {
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.247, pin_center_inch(all_formats[0], 1));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.847, pin_center_inch(all_formats[0], 5));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.231, pin_center_inch(all_formats[1], 1));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.012, pin_center_inch(all_formats[1], 6));
}

void test_kw1_shoulder_and_elbow(void) {
    uint8_t d[6] = {1, 1, 1, 1, 1, 1};
    Segment segs[128];
    int n = build_contour(all_formats[0], d, segs, 128);
    TEST_ASSERT_TRUE(n > 4);
    double half = 0.084 / 2.0;
    TEST_ASSERT_TRUE(count_near(segs, n, 0.0, 0.0, 0.247 - half, 0.0) >= 1);
    double level = 0.847 + 0.15;
    TEST_ASSERT_TRUE(count_near(segs, n, 0.0, 0.329, level, 0.329) >= 1);
    TEST_ASSERT_TRUE(count_near(segs, n, level, 0.329, level + 0.15, 0.329 - 0.15) >= 1);
}

void test_h75_has_bottom_edge(void) {
    int hi = find_format_by_name("H75");
    uint8_t d[16];
    for (int i = 0; i < 16; i++) d[i] = (uint8_t)all_formats[hi].min_depth_ind;
    Segment segs[256];
    int n = build_contour(all_formats[hi], d, segs, 256);
    int bottomish = 0;
    double uncut = all_formats[hi].uncut_depth_inch;
    for (int i = 0; i < n; i++) {
        if (fabs(segs[i].y0 - uncut) < 0.002 && fabs(segs[i].y1 - uncut) < 0.002) bottomish++;
    }
    TEST_ASSERT_TRUE(bottomish >= 1);
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_native_runner_works);
    RUN_TEST(test_format_count_is_23);
    RUN_TEST(test_kw1_fields);
    RUN_TEST(test_sc4_pin_count);
    RUN_TEST(test_h75_is_double_sided);
    RUN_TEST(test_find_format_unknown);
    RUN_TEST(test_macs_rejects_out_of_range);
    RUN_TEST(test_macs_allows_legal_first_pin);
    RUN_TEST(test_macs_rejects_illegal_adjacent);
    RUN_TEST(test_macs_middle_pin_checks_both_neighbors);
    RUN_TEST(test_pin_centers_kw1_sc4);
    RUN_TEST(test_kw1_shoulder_and_elbow);
    RUN_TEST(test_h75_has_bottom_edge);
    return UNITY_END();
}
