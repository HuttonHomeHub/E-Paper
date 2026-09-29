/**
 * Loads the bin collection schedule from the calendar feed over WiFi.
 *
 * This is the hardware/network half of the Bin Collection page: it joins WiFi,
 * sets the clock (UkClock), downloads the .ics file and hands it to
 * IcsBins_Parse(). Credentials and the feed URL come from include/secrets.h
 * (see secrets.example.h).
 *
 * A good download is saved to flash. If a later refresh fails, the saved copy
 * is shown instead, flagged with an "OUT OF DATE" warning, so a WiFi outage
 * degrades the page rather than blanking it.
 */
#ifndef BIN_FEED_H
#define BIN_FEED_H

#include "bin_data.h"

/* True when secrets.h exists and has a WiFi name, i.e. a live load is possible. */
bool BinFeed_Configured();

/* Fills `view` with today's date and the collections: fresh from the feed
 * (view->warning NULL) or, if the feed can't be reached, the last saved copy
 * (view->warning says it is out of date). The view points at static storage,
 * valid until the next call. Turns WiFi off again before returning.
 *
 * Returns false, with a short screen-sized reason in *error, only when there is
 * nothing usable: no fresh data and no saved copy, or the date is unknown. */
bool BinFeed_Load(BinView *view, const char **error);

#endif
