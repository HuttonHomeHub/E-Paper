#include <string.h>
#include <unity.h>

#include "bin_schedule.h"
#include "calendar_date.h"

void setUp(void) {}
void tearDown(void) {}

static BinView viewOf(const BinCollection *c, int n)
{
    BinView v;
    v.todayYear = 2026; v.todayMonth = 8; v.todayDay = 24;   /* a Monday */
    v.collections = c;
    v.collectionCount = n;
    v.statusLine = "";
    return v;
}

static void test_groups_are_sorted_and_merged(void)
{
    const BinCollection c[] = {
        { 2026, 9, 1,  BIN_BLUE  },     /* deliberately out of order */
        { 2026, 8, 25, BIN_BLACK },
        { 2026, 9, 1,  BIN_FOOD  },
        { 2026, 8, 25, BIN_FOOD  },
    };
    BinView v = viewOf(c, 4);
    BinGroup g[BIN_MAX_GROUPS];

    TEST_ASSERT_EQUAL_INT(2, BinSchedule_Build(&v, g, BIN_MAX_GROUPS));
    TEST_ASSERT_EQUAL_INT(1, g[0].daysAway);
    TEST_ASSERT_EQUAL_INT(8, g[1].daysAway);
    TEST_ASSERT_EQUAL_UINT((1u << BIN_BLACK) | (1u << BIN_FOOD), g[0].mask);
    TEST_ASSERT_EQUAL_UINT((1u << BIN_BLUE)  | (1u << BIN_FOOD), g[1].mask);
}

static void test_today_is_included_and_past_is_dropped(void)
{
    const BinCollection c[] = {
        { 2026, 8, 23, BIN_RED   },     /* yesterday */
        { 2026, 8, 24, BIN_BLACK },     /* today     */
    };
    BinView v = viewOf(c, 2);
    BinGroup g[BIN_MAX_GROUPS];

    TEST_ASSERT_EQUAL_INT(1, BinSchedule_Build(&v, g, BIN_MAX_GROUPS));
    TEST_ASSERT_EQUAL_INT(0, g[0].daysAway);
    TEST_ASSERT_EQUAL_UINT(1u << BIN_BLACK, g[0].mask);
}

static void test_year_rollover_counts_days(void)
{
    const BinCollection c[] = { { 2027, 1, 1, BIN_GARDEN } };
    BinView v = viewOf(c, 1);
    v.todayMonth = 12; v.todayDay = 30;
    BinGroup g[BIN_MAX_GROUPS];

    TEST_ASSERT_EQUAL_INT(1, BinSchedule_Build(&v, g, BIN_MAX_GROUPS));
    TEST_ASSERT_EQUAL_INT(2, g[0].daysAway);
}

static void test_group_limit_keeps_the_earliest_days(void)
{
    BinCollection c[6];
    for (int i = 0; i < 6; i++) {
        c[i].year = 2026; c[i].month = 9; c[i].day = 6 - i;   /* 6,5,4,3,2,1 Sep */
        c[i].bin = BIN_BLUE;
    }
    BinView v = viewOf(c, 6);
    BinGroup g[3];

    TEST_ASSERT_EQUAL_INT(3, BinSchedule_Build(&v, g, 3));
    TEST_ASSERT_EQUAL_INT32(Cal_DaysFromCivil(2026, 9, 1), g[0].serial);
    TEST_ASSERT_EQUAL_INT32(Cal_DaysFromCivil(2026, 9, 2), g[1].serial);
    TEST_ASSERT_EQUAL_INT32(Cal_DaysFromCivil(2026, 9, 3), g[2].serial);
}

static void test_invalid_bin_and_empty_input_are_safe(void)
{
    const BinCollection c[] = { { 2026, 8, 25, (BinType)99 } };
    BinView v = viewOf(c, 1);
    BinGroup g[BIN_MAX_GROUPS];

    TEST_ASSERT_EQUAL_INT(0, BinSchedule_Build(&v, g, BIN_MAX_GROUPS));
    v.collectionCount = 0;
    TEST_ASSERT_EQUAL_INT(0, BinSchedule_Build(&v, g, BIN_MAX_GROUPS));
    TEST_ASSERT_EQUAL_INT(0, BinSchedule_Build(NULL, g, BIN_MAX_GROUPS));
}

static void test_mask_helpers(void)
{
    const unsigned m = (1u << BIN_BLUE) | (1u << BIN_GARDEN);
    TEST_ASSERT_EQUAL_INT(2, BinSchedule_CountBins(m));
    TEST_ASSERT_TRUE(BinSchedule_HasBin(m, BIN_GARDEN));
    TEST_ASSERT_FALSE(BinSchedule_HasBin(m, BIN_RED));
    TEST_ASSERT_EQUAL_INT(0, BinSchedule_CountBins(0));
}

static void test_labels(void)
{
    char b[32];
    Bin_HeroLabel(0, 0, b, sizeof b); TEST_ASSERT_EQUAL_STRING("TODAY", b);
    Bin_HeroLabel(1, 1, b, sizeof b); TEST_ASSERT_EQUAL_STRING("TOMORROW", b);
    Bin_HeroLabel(3, 2, b, sizeof b); TEST_ASSERT_EQUAL_STRING("WEDNESDAY", b);
    Bin_HeroLabel(6, 5, b, sizeof b); TEST_ASSERT_EQUAL_STRING("SATURDAY", b);
    Bin_HeroLabel(7, 0, b, sizeof b); TEST_ASSERT_EQUAL_STRING("IN 7 DAYS", b);   /* not "MONDAY" */
    Bin_HeroLabel(3, 2, b, 5);        TEST_ASSERT_EQUAL_STRING("WEDN", b);   /* truncates safely */
    Bin_Advice(0, b, sizeof b);       TEST_ASSERT_EQUAL_STRING("Collection day", b);
    Bin_Advice(1, b, sizeof b);       TEST_ASSERT_EQUAL_STRING("Put bins out tonight", b);
    Bin_Advice(5, b, sizeof b);       TEST_ASSERT_EQUAL_STRING("In 5 days", b);
    Bin_Advice(9, b, sizeof b);       TEST_ASSERT_EQUAL_STRING("Not this week", b);
    Bin_DaysAwayText(8, b, sizeof b); TEST_ASSERT_EQUAL_STRING("In 8 days", b);
    TEST_ASSERT_EQUAL_STRING("Garden", Bin_Name(BIN_GARDEN));
    TEST_ASSERT_EQUAL_STRING("", Bin_Name((BinType)42));
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_groups_are_sorted_and_merged);
    RUN_TEST(test_today_is_included_and_past_is_dropped);
    RUN_TEST(test_year_rollover_counts_days);
    RUN_TEST(test_group_limit_keeps_the_earliest_days);
    RUN_TEST(test_invalid_bin_and_empty_input_are_safe);
    RUN_TEST(test_mask_helpers);
    RUN_TEST(test_labels);
    return UNITY_END();
}
