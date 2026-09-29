/**
 * Loads the bin collection schedule from the calendar feed over WiFi.
 *
 * This is the hardware/network half of the Bin Collection page: it joins WiFi,
 * sets the clock over NTP (UK time), downloads the .ics file and hands it to
 * IcsBins_Parse(). Credentials and the feed URL come from include/secrets.h
 * (see secrets.example.h).
 */
#ifndef BIN_FEED_H
#define BIN_FEED_H

#include "bin_data.h"

/* True when secrets.h exists and has a WiFi name, i.e. a live load is possible. */
bool BinFeed_Configured();

/* Fills `view` with today's date and the collections from the feed. The view
 * points at static storage, valid until the next call. Turns WiFi off again
 * before returning. On failure returns false and sets *error to a short,
 * screen-sized reason; `view` is then not usable. */
bool BinFeed_Load(BinView *view, const char **error);

#endif
