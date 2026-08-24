#include "dummy_data.h"

/* A fixed "today" so the layout renders identically every time and can be
 * compared between runs. The live version will take this from the clock. */
#define DUMMY_YEAR   2026
#define DUMMY_MONTH  8
#define DUMMY_DAY    24

/* Deliberately includes same-day clashes, all-day entries and an over-long
 * title, so the layout is exercised rather than flattered. */
static const CalEvent kEvents[] = {
    { 2026,  8, 24,  8 * 60 + 30,  9 * 60 +  0, "School run" },
    { 2026,  8, 24, 13 * 60 +  0, 13 * 60 + 45, "Dentist - Luna" },
    { 2026,  8, 24, 19 * 60 +  0,           -1, "Bins out" },
    { 2026,  8, 25,  9 * 60 + 30, 10 * 60 + 30, "Standup" },
    { 2026,  8, 25, 14 * 60 +  0, 15 * 60 + 30, "Quarterly planning review with the wider team" },
    { 2026,  8, 26,  CAL_ALL_DAY,           -1, "Jo away - Manchester" },
    { 2026,  8, 26, 18 * 60 + 15,           -1, "Swimming" },
    { 2026,  8, 27, 12 * 60 +  0, 13 * 60 +  0, "Lunch with Sam" },
    { 2026,  8, 28,  CAL_ALL_DAY,           -1, "Bank holiday" },
    { 2026,  8, 29, 10 * 60 +  0, 11 * 60 +  0, "Car service" },
    { 2026,  8, 31,  CAL_ALL_DAY,           -1, "Luna birthday" },
    { 2026,  9,  2,  9 * 60 +  0,           -1, "Term starts" },
    { 2026,  9,  4, 19 * 60 + 30, 22 * 60 +  0, "Dinner - the Harpers" },
};

void DummyData_Fill(CalView *view)
{
    view->todayYear  = DUMMY_YEAR;
    view->todayMonth = DUMMY_MONTH;
    view->todayDay   = DUMMY_DAY;
    view->nowMin     = 14 * 60 + 32;

    view->events     = kEvents;
    view->eventCount = (int)(sizeof(kEvents) / sizeof(kEvents[0]));

    view->statusLine = "Updated 14:32 - placeholder data";
}
