/**
 * Reads bin collections out of an iCalendar (.ics) feed such as the one the
 * council publishes through ReCollect. Pure C++ so it is unit tested on the
 * host.
 *
 * The feed has one all-day VEVENT per collection day, and its SUMMARY names
 * what is collected rather than which colour bin, e.g.
 *
 *   Food waste, plastic recycling (blue-lid bin or clear sacks), and refuse
 *
 * so one event can mean several bins. IcsBins_FromSummary() holds the keyword
 * table that maps that wording to bins.
 */
#ifndef ICS_BINS_H
#define ICS_BINS_H

#include <stddef.h>

#include "bin_data.h"

/* Bins named by an event summary, as a bit per BinType (1u << bin); 0 if the
 * wording is not recognised. Matching is case-insensitive. */
unsigned IcsBins_FromSummary(const char *summary);

/* Parses `len` bytes of ICS text into `out` (up to maxOut entries) and returns
 * how many were written. Each VEVENT yields one BinCollection per bin it names;
 * events naming no known bin are skipped. Handles CRLF or LF, folded lines,
 * escaped commas and DTSTART as a date or a date-time (the date part is used).
 * Output follows feed order; BinSchedule_Build() does the sorting. */
int IcsBins_Parse(const char *ics, size_t len, BinCollection *out, int maxOut);

#endif
