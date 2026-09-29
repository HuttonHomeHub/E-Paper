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
    { 2026,  8, 24, 13 * 60 +  0, 13 * 60 + 45, "Dentist appointment" },
    { 2026,  8, 24, 19 * 60 +  0,           -1, "Bins out" },
    { 2026,  8, 25,  9 * 60 + 30, 10 * 60 + 30, "Standup" },
    { 2026,  8, 25, 14 * 60 +  0, 15 * 60 + 30, "Quarterly planning review with the wider team" },
    { 2026,  8, 26,  CAL_ALL_DAY,           -1, "Away - conference" },
    { 2026,  8, 26, 18 * 60 + 15,           -1, "Swimming" },
    { 2026,  8, 27, 12 * 60 +  0, 13 * 60 +  0, "Lunch with a friend" },
    { 2026,  8, 28,  CAL_ALL_DAY,           -1, "Bank holiday" },
    { 2026,  8, 29, 10 * 60 +  0, 11 * 60 +  0, "Car service" },
    { 2026,  8, 31,  CAL_ALL_DAY,           -1, "Family birthday" },
    { 2026,  9,  2,  9 * 60 +  0,           -1, "Term starts" },
    { 2026,  9,  4, 19 * 60 + 30, 22 * 60 +  0, "Dinner with friends" },
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

/* A plausible fortnightly rota, collected on Tuesdays: food every week, black
 * and blue/red on alternate weeks, garden every other week. Includes a day with
 * three bins and one with two so the layout is exercised at both counts. */
static const BinCollection kBins[] = {
    { 2026,  8, 25, BIN_BLACK  }, { 2026,  8, 25, BIN_FOOD },
    { 2026,  9,  1, BIN_BLUE   }, { 2026,  9,  1, BIN_FOOD }, { 2026,  9,  1, BIN_GARDEN },
    { 2026,  9,  8, BIN_BLACK  }, { 2026,  9,  8, BIN_FOOD },
    { 2026,  9, 15, BIN_RED    }, { 2026,  9, 15, BIN_FOOD }, { 2026,  9, 15, BIN_GARDEN },
    { 2026,  9, 22, BIN_BLACK  }, { 2026,  9, 22, BIN_FOOD },
    { 2026,  9, 29, BIN_BLUE   }, { 2026,  9, 29, BIN_FOOD }, { 2026,  9, 29, BIN_GARDEN },
};

void DummyBinData_Fill(BinView *view)
{
    view->todayYear  = DUMMY_YEAR;
    view->todayMonth = DUMMY_MONTH;
    view->todayDay   = DUMMY_DAY;

    view->collections     = kBins;
    view->collectionCount = (int)(sizeof(kBins) / sizeof(kBins[0]));

    view->statusLine = "Updated 14:32 - placeholder data";
}
