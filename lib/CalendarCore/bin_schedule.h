/**
 * Turns a flat list of bin collections into the per-day groups the screen
 * shows. Pure logic, unit tested on the host.
 */
#ifndef BIN_SCHEDULE_H
#define BIN_SCHEDULE_H

#include <stddef.h>

#include "bin_data.h"

#define BIN_MAX_GROUPS 8

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

/* Big headline for a collection day: "TODAY", "TOMORROW", the upper-case
 * weekday name ("WEDNESDAY") within the week, else "IN 9 DAYS". dowMon0 is
 * 0=Mon .. 6=Sun. */
void Bin_HeroLabel(int daysAway, int dowMon0, char *buf, size_t len);

/* What to do about it: "Collection day", "Put bins out tonight", "In 5 days",
 * or "Not this week" from a week out. */
void Bin_Advice(int daysAway, char *buf, size_t len);

/* Compact countdown for the list: "Today", "Tomorrow", "In 8 days". */
void Bin_DaysAwayText(int daysAway, char *buf, size_t len);

#endif
