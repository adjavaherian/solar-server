#include <Unity.h>

#ifdef UNIT_TEST

void tests_are_working(void) {
    TEST_ASSERT_EQUAL(true, true);
}

void setup() {
    UNITY_BEGIN();    // IMPORTANT LINE!
    // RUN_TEST(tests_are_working);
}

void loop() {
    RUN_TEST(tests_are_working);
    UNITY_END(); // stop unit testing
}

#endif
