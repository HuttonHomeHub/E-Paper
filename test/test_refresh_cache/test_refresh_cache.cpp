#include <unity.h>

#include "bin_cache.h"
#include "refresh_schedule.h"

void setUp(void) {}
void tearDown(void) {}

/* ---------------------------------------------------------- refresh timing -- */

static void test_wait_until_the_next_refresh(void)
{
    TEST_ASSERT_EQUAL_INT32(43500, Refresh_SecondsUntilNext(12, 0, 0));    /* 12h05m */
    TEST_ASSERT_EQUAL_INT32(360,   Refresh_SecondsUntilNext(23, 59, 0));   /* across midnight */
    TEST_ASSERT_EQUAL_INT32(300,   Refresh_SecondsUntilNext(0, 0, 0));
    TEST_ASSERT_EQUAL_INT32(83100, Refresh_SecondsUntilNext(1, 0, 0));
}

static void test_the_refresh_instant_itself_waits_a_full_day(void)
{
    TEST_ASSERT_EQUAL_INT32(1,     Refresh_SecondsUntilNext(0, 4, 59));
    TEST_ASSERT_EQUAL_INT32(86400, Refresh_SecondsUntilNext(0, 5, 0));
    TEST_ASSERT_EQUAL_INT32(86399, Refresh_SecondsUntilNext(0, 5, 1));
}

static void test_wait_is_always_positive_and_at_most_a_day(void)
{
    for (int h = 0; h < 24; h++) {
        for (int m = 0; m < 60; m += 7) {
            const long s = Refresh_SecondsUntilNext(h, m, 30);
            TEST_ASSERT_TRUE(s > 0 && s <= 86400L);
        }
    }
}

/* ------------------------------------------------------------- cache codec -- */

static void test_pack_round_trips_every_bin(void)
{
    for (int b = 0; b < BIN_TYPE_COUNT; b++) {
        const BinCollection in = { 2026, 12, 31, (BinType)b };
        BinCollection out = { 0, 0, 0, BIN_BLUE };
        TEST_ASSERT_TRUE(BinCache_Unpack(BinCache_Pack(&in), &out));
        TEST_ASSERT_EQUAL_INT(2026, out.year);
        TEST_ASSERT_EQUAL_INT(12,   out.month);
        TEST_ASSERT_EQUAL_INT(31,   out.day);
        TEST_ASSERT_EQUAL_INT(b,    out.bin);
    }
}

static void test_unpack_rejects_corrupt_entries(void)
{
    BinCollection c;
    TEST_ASSERT_FALSE(BinCache_Unpack(0, &c));                     /* empty flash / zeroed */
    TEST_ASSERT_FALSE(BinCache_Unpack(0xFFFFFFFFu, &c));           /* erased flash          */
    TEST_ASSERT_FALSE(BinCache_Unpack(202601019u, &c));            /* bin digit 9           */
    TEST_ASSERT_FALSE(BinCache_Unpack(202613010u, &c));            /* month 13              */
    TEST_ASSERT_FALSE(BinCache_Unpack(202601320u, &c));            /* day 32                */
    TEST_ASSERT_FALSE(BinCache_Unpack(202602300u, &c));            /* 30 February           */
    TEST_ASSERT_FALSE(BinCache_Unpack(199912010u, &c));            /* year before 2000      */
    TEST_ASSERT_TRUE(BinCache_Unpack(202402290u, &c));             /* 29 Feb, leap year     */
}

static void test_date_key(void)
{
    TEST_ASSERT_EQUAL_UINT32(20260929u, BinCache_DateKey(2026, 9, 29));
    TEST_ASSERT_EQUAL_UINT32(20270105u, BinCache_DateKey(2027, 1, 5));
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_wait_until_the_next_refresh);
    RUN_TEST(test_the_refresh_instant_itself_waits_a_full_day);
    RUN_TEST(test_wait_is_always_positive_and_at_most_a_day);
    RUN_TEST(test_pack_round_trips_every_bin);
    RUN_TEST(test_unpack_rejects_corrupt_entries);
    RUN_TEST(test_date_key);
    return UNITY_END();
}
