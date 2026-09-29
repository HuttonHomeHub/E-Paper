#include <string.h>
#include <unity.h>

#include "ics_bins.h"

void setUp(void) {}
void tearDown(void) {}

#define MASK(b) (1u << (b))

static void test_summary_wording_from_the_real_feed(void)
{
    TEST_ASSERT_EQUAL_UINT(MASK(BIN_GARDEN), IcsBins_FromSummary("Garden"));
    TEST_ASSERT_EQUAL_UINT(MASK(BIN_FOOD) | MASK(BIN_BLUE) | MASK(BIN_BLACK),
        IcsBins_FromSummary("Food waste, plastic recycling  (blue-lid bin or clear sacks), "
                            "and refuse (household rubbish)"));
    TEST_ASSERT_EQUAL_UINT(MASK(BIN_FOOD) | MASK(BIN_RED),
        IcsBins_FromSummary("Food waste and paper and card (red-lid bin or red sacks)"));
    TEST_ASSERT_EQUAL_UINT(MASK(BIN_RED),
        IcsBins_FromSummary("Paper and card (red-lid bin or red sacks)"));
}

static void test_summary_is_case_insensitive_and_accepts_colour_names(void)
{
    TEST_ASSERT_EQUAL_UINT(MASK(BIN_BLUE),  IcsBins_FromSummary("BLUE BIN"));
    TEST_ASSERT_EQUAL_UINT(MASK(BIN_BLACK), IcsBins_FromSummary("black bin"));
    TEST_ASSERT_EQUAL_UINT(MASK(BIN_RED),   IcsBins_FromSummary("Red bin"));
}

static void test_colour_words_need_whole_word_matches(void)
{
    TEST_ASSERT_EQUAL_UINT(0, IcsBins_FromSummary("Shredded reduced bluebell"));
    TEST_ASSERT_EQUAL_UINT(0, IcsBins_FromSummary("Bank holiday - no changes"));
    TEST_ASSERT_EQUAL_UINT(0, IcsBins_FromSummary(""));
    TEST_ASSERT_EQUAL_UINT(0, IcsBins_FromSummary(NULL));
}

/* Trimmed and anonymised copy of the real feed's shape: CRLF line ends, folded
 * lines, escaped commas, all-day DTSTART, and an unrelated event. */
static const char kFeed[] =
    "BEGIN:VCALENDAR\r\n"
    "VERSION:2.0\r\n"
    "X-WR-CALNAME:Example Street\\, Town\r\n"
    "BEGIN:VEVENT\r\n"
    "DESCRIPTION:Garden\r\n"
    "DTSTART;VALUE=DATE:20260929\r\n"
    "SUMMARY:Garden\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20261002\r\n"
    "SUMMARY:Food waste\\, plastic recycling  (blue-lid bin or clear sacks)\\, and\r\n"
    "  refuse (household rubbish)\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20261009\r\n"
    "SUMMARY:Food waste and paper and card (red-lid bin or red sacks)\r\n"
    "END:VEVENT\r\n"
    "BEGIN:VEVENT\r\n"
    "DTSTART;VALUE=DATE:20261010\r\n"
    "SUMMARY:Village fete\r\n"
    "END:VEVENT\r\n"
    "END:VCALENDAR\r\n";

static void test_parses_events_into_one_entry_per_bin(void)
{
    BinCollection c[16];
    const int n = IcsBins_Parse(kFeed, sizeof(kFeed) - 1, c, 16);

    /* 1 garden + (food, blue, black) + (food, red); the fete is skipped. */
    TEST_ASSERT_EQUAL_INT(6, n);

    TEST_ASSERT_EQUAL_INT(2026, c[0].year);
    TEST_ASSERT_EQUAL_INT(9,    c[0].month);
    TEST_ASSERT_EQUAL_INT(29,   c[0].day);
    TEST_ASSERT_EQUAL_INT(BIN_GARDEN, c[0].bin);

    /* Folded summary was rejoined, so all three bins were found; enum order. */
    TEST_ASSERT_EQUAL_INT(BIN_BLUE,  c[1].bin);
    TEST_ASSERT_EQUAL_INT(BIN_BLACK, c[2].bin);
    TEST_ASSERT_EQUAL_INT(BIN_FOOD,  c[3].bin);
    TEST_ASSERT_EQUAL_INT(10, c[1].month);
    TEST_ASSERT_EQUAL_INT(2,  c[1].day);

    TEST_ASSERT_EQUAL_INT(BIN_RED,  c[4].bin);
    TEST_ASSERT_EQUAL_INT(BIN_FOOD, c[5].bin);
    TEST_ASSERT_EQUAL_INT(9, c[5].day);
}

static void test_lf_endings_and_datetime_dtstart(void)
{
    static const char ics[] =
        "BEGIN:VEVENT\n"
        "DTSTART;TZID=Europe/London:20261016T070000\n"
        "SUMMARY:Garden\n"
        "END:VEVENT\n";
    BinCollection c[4];
    TEST_ASSERT_EQUAL_INT(1, IcsBins_Parse(ics, sizeof(ics) - 1, c, 4));
    TEST_ASSERT_EQUAL_INT(2026, c[0].year);
    TEST_ASSERT_EQUAL_INT(10,   c[0].month);
    TEST_ASSERT_EQUAL_INT(16,   c[0].day);
}

static void test_bad_dates_and_missing_pieces_are_skipped(void)
{
    static const char ics[] =
        "BEGIN:VEVENT\nDTSTART;VALUE=DATE:20261341\nSUMMARY:Garden\nEND:VEVENT\n"   /* month 13 */
        "BEGIN:VEVENT\nDTSTART;VALUE=DATE:20260230\nSUMMARY:Garden\nEND:VEVENT\n"   /* 30 Feb   */
        "BEGIN:VEVENT\nSUMMARY:Garden\nEND:VEVENT\n"                                /* no date  */
        "BEGIN:VEVENT\nDTSTART;VALUE=DATE:20261016\nEND:VEVENT\n"                   /* no title */
        "DTSTART;VALUE=DATE:20261017\nSUMMARY:Garden\n";                            /* no event */
    BinCollection c[4];
    TEST_ASSERT_EQUAL_INT(0, IcsBins_Parse(ics, sizeof(ics) - 1, c, 4));
}

static void test_output_is_capped_and_inputs_are_checked(void)
{
    BinCollection c[2];
    TEST_ASSERT_EQUAL_INT(2, IcsBins_Parse(kFeed, sizeof(kFeed) - 1, c, 2));
    TEST_ASSERT_EQUAL_INT(0, IcsBins_Parse(kFeed, sizeof(kFeed) - 1, c, 0));
    TEST_ASSERT_EQUAL_INT(0, IcsBins_Parse(NULL, 10, c, 2));
    TEST_ASSERT_EQUAL_INT(0, IcsBins_Parse(kFeed, 0, c, 2));
}

static void test_overlong_line_is_truncated_not_overrun(void)
{
    static char ics[2000];
    strcpy(ics, "BEGIN:VEVENT\nDTSTART;VALUE=DATE:20261016\nSUMMARY:Garden ");
    size_t n = strlen(ics);
    memset(ics + n, 'x', 1500);
    strcpy(ics + n + 1500, "\nEND:VEVENT\n");

    BinCollection c[4];
    TEST_ASSERT_EQUAL_INT(1, IcsBins_Parse(ics, strlen(ics), c, 4));
    TEST_ASSERT_EQUAL_INT(BIN_GARDEN, c[0].bin);
}

int main(int, char **)
{
    UNITY_BEGIN();
    RUN_TEST(test_summary_wording_from_the_real_feed);
    RUN_TEST(test_summary_is_case_insensitive_and_accepts_colour_names);
    RUN_TEST(test_colour_words_need_whole_word_matches);
    RUN_TEST(test_parses_events_into_one_entry_per_bin);
    RUN_TEST(test_lf_endings_and_datetime_dtstart);
    RUN_TEST(test_bad_dates_and_missing_pieces_are_skipped);
    RUN_TEST(test_output_is_capped_and_inputs_are_checked);
    RUN_TEST(test_overlong_line_is_truncated_not_overrun);
    return UNITY_END();
}
