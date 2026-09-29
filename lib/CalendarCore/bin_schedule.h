/**
 * Turns a flat list of bin collections into the per-day groups the screen
 * shows. Pure logic, unit tested on the host.
 */
#ifndef BIN_SCHEDULE_H
#define BIN_SCHEDULE_H

#include <stddef.h>

#include "bin_data.h"

#define BIN_MAX_GROUPS 8

/* The calendar-ending banner appears once the last date in the data is this
 * close (or already past). */
#define BIN_ENDS_SOON_DAYS 14

/* All the bins collected on one day. */
typedef struct {
    long     serial;        /* Cal_DaysFromCivil() of the collection day */
    int      daysAway;      /* 0 = today, 1 = tomorrow, ... */
    unsigned mask;          /* bit (1u << BinType) per bin collected */
} BinGroup;

/* Fills `out` (up to maxGroups) with today's and future collection days in
 * date order, merging bins that share a day. Unsorted input is fine; invalid
 * bin types are skipped. Returns the number of groups written. */
int BinSchedule_Build(const BinView *view, BinGroup *out, int maxGroups);

int BinSchedule_CountBins(unsigned mask);
int BinSchedule_HasBin(unsigned mask, BinType bin);

/* The bin's display name, e.g. "Blue"; "" for an invalid type. */
const char *Bin_Name(BinType bin);

/* What goes in the bin, short enough to caption an icon: "Glass & cans",
 * "General", "Cardboard", "Food waste", "Garden waste"; "" for an invalid type. */
const char *Bin_Contents(BinType bin);

/* Big headline for a collection day: "TODAY", "TOMORROW", the upper-case
 * weekday name ("WEDNESDAY") within the week, else "IN 9 DAYS". dowMon0 is
 * 0=Mon .. 6=Sun. */
void Bin_HeroLabel(int daysAway, int dowMon0, char *buf, size_t len);

/* What to do about it: "Collection day", "Put bins out tonight", "In 5 days",
 * or "Not this week" from a week out. */
void Bin_Advice(int daysAway, char *buf, size_t len);

/* Banner text for data that could not be refreshed, given the date it was last
 * good: "OUT OF DATE - last updated Tue 27 Sep". */
void Bin_StaleWarning(char *buf, size_t len, int year, int month, int day);

/* If the last collection date in `view` is within BIN_ENDS_SOON_DAYS of today,
 * or already past, writes banner text and returns 1; otherwise returns 0:
 *   "CALENDAR ENDS SOON - last date Fri 26 Feb"
 *   "CALENDAR ENDED - last date Fri 26 Feb"
 * Invalid bin types are ignored, as elsewhere. */
int BinSchedule_CalendarEnd(const BinView *view, char *buf, size_t len);

/* Compact countdown for the list: "Today", "Tomorrow", "In 8 days". */
void Bin_DaysAwayText(int daysAway, char *buf, size_t len);

#endif
