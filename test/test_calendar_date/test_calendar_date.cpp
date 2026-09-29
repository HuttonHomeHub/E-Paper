#include <unity.h>

#include "calendar_date.h"

void setUp(void) {}
void tearDown(void) {}

static void test_day_of_week_is_monday_zero(void)
{
    TEST_ASSERT_EQUAL_INT(0, Cal_DayOfWeekMon0(2026, 8, 24));  /* Monday   */
    TEST_ASSERT_EQUAL_INT(5, Cal_DayOfWeekMon0(2026, 8, 1));   /* Saturday */
    TEST_ASSERT_EQUAL_INT(5, Cal_DayOfWeekMon0(2000, 1, 1));   /* Saturday */
    TEST_ASSERT_EQUAL_INT(3, Cal_DayOfWeekMon0(1970, 1, 1));   /* Thursday */
    TEST_ASSERT_EQUAL_INT(3, Cal_DayOfWeekMon0(2024, 2, 29));  /* Thursday */
}

static void test_days_in_month_handles_leap_years(void)
{
    TEST_ASSERT_EQUAL_INT(29, Cal_DaysInMonth(2024, 2));
    TEST_ASSERT_EQUAL_INT(28, Cal_DaysInMonth(2100, 2));  /* century, not leap */
    TEST_ASSERT_EQUAL_INT(29, Cal_DaysInMonth(2000, 2));  /* 400th year, leap  */
    TEST_ASSERT_EQUAL_INT(31, Cal_DaysInMonth(2026, 8));
}

static void test_serial_day_numbers(void)
{
    TEST_ASSERT_EQUAL_INT32(0,     Cal_DaysFromCivil(1970, 1, 1));
    TEST_ASSERT_EQUAL_INT32(20689, Cal_DaysFromCivil(2026, 8, 24));
}

static void test_serial_round_trips_over_wide_range(void)
{
    for (long s = -40000; s < 40000; s += 7) {
        int y, m, d;
        Cal_CivilFromDays(s, &y, &m, &d);
        TEST_ASSERT_EQUAL_INT32_MESSAGE(s, Cal_DaysFromCivil(y, m, d), "round trip failed");
    }
}

static void test_year_rollover(void)
{
    int y, m, d;
    Cal_CivilFromDays(Cal_DaysFromCivil(2026, 12, 31) + 1, &y, &m, &d);
    TEST_ASSERT_EQUAL_INT(2027, y);
    TEST_ASSERT_EQUAL_INT(1, m);
    TEST_ASSERT_EQUAL_INT(1, d);
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_day_of_week_is_monday_zero);
    RUN_TEST(test_days_in_month_handles_leap_years);
    RUN_TEST(test_serial_day_numbers);
    RUN_TEST(test_serial_round_trips_over_wide_range);
    RUN_TEST(test_year_rollover);
    return UNITY_END();
}
