/**
 * Data model for the calendar screen.
 *
 * Deliberately free of Arduino and network types so the same structs can be
 * filled from dummy data, from a live feed later, or from a host-side test
 * harness that renders the screen to an image.
 */
#ifndef CALENDAR_DATA_H
#define CALENDAR_DATA_H

#define CAL_MAX_EVENTS      64
#define CAL_TITLE_LEN       64

/* One calendar entry. Times are minutes from midnight so the renderer never
 * has to think about time zones; whoever fills these in has already resolved
 * that. startMin == CAL_ALL_DAY marks an all-day event. */
#define CAL_ALL_DAY (-1)

typedef struct {
    int  year;
    int  month;             /* 1-12 */
    int  day;               /* 1-31 */
    int  startMin;          /* minutes from midnight, or CAL_ALL_DAY */
    int  endMin;            /* minutes from midnight, or -1 if unknown */
    char title[CAL_TITLE_LEN];
} CalEvent;

/* Everything the screen needs for one render. */
typedef struct {
    int      todayYear;
    int      todayMonth;    /* 1-12 */
    int      todayDay;      /* 1-31 */
    int      nowMin;        /* current time, minutes from midnight, -1 to hide */

    const CalEvent *events; /* sorted ascending by date then time */
    int      eventCount;

    const char *statusLine; /* small text in the header, e.g. "Updated 14:32" */
} CalView;

#endif
