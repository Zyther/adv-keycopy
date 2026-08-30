#include <unity.h>
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
    return UNITY_END();
}
