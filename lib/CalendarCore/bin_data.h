/**
 * Data model for the bin collection screen.
 *
 * Like calendar_data.h this is free of Arduino and network types, so it can be
 * filled from dummy data, from a calendar feed later, or from a test.
 */
#ifndef BIN_DATA_H
#define BIN_DATA_H

/* Order here is the order bins are shown when several are collected together. */
typedef enum {
    BIN_BLUE = 0,
    BIN_BLACK,
    BIN_RED,
    BIN_FOOD,
    BIN_GARDEN,
    BIN_TYPE_COUNT
} BinType;

/* One bin being collected on one day. Several entries may share a date. */
typedef struct {
    int     year;
    int     month;          /* 1-12 */
    int     day;            /* 1-31 */
    BinType bin;
} BinCollection;

/* Everything the screen needs for one render. */
typedef struct {
    int      todayYear;
    int      todayMonth;    /* 1-12 */
    int      todayDay;      /* 1-31 */

    const BinCollection *collections;   /* any order; past dates are ignored */
    int      collectionCount;

    const char *statusLine; /* small text in the header, e.g. "Updated 14:32" */
} BinView;

#endif
