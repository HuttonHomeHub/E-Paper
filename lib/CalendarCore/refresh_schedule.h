/**
 * When the display should next wake by itself.
 *
 * The pages show dates, never the time of day, so the only moment they go out
 * of date is midnight ("TOMORROW" becomes "TODAY"). The board therefore sleeps
 * until just after midnight and redraws. If the data could not be refreshed, it
 * also wakes at growing intervals to try again quietly, redrawing only if a try
 * succeeds. Pure arithmetic, unit tested.
 */
#ifndef REFRESH_SCHEDULE_H
#define REFRESH_SCHEDULE_H

/* Local time of the daily refresh. A few minutes past midnight rather than on
 * it, so a wake that comes slightly early (the ESP32's sleep timer is not a
 * precision clock) still lands on the new day. */
#ifndef REFRESH_HOUR
#define REFRESH_HOUR    0
#endif
#ifndef REFRESH_MINUTE
#define REFRESH_MINUTE  5
#endif
/* (Both can be overridden with -D for a bench test, e.g. a refresh a few minutes
 * from now, instead of waiting for midnight.) */

/* One retry "unit". Retries come after 1, 2 and 4 units, then every unit. It
 * is an hour; override with -DREFRESH_RETRY_UNIT_SECONDS=30 to test the
 * behaviour on a bench without waiting hours. */
#ifndef REFRESH_RETRY_UNIT_SECONDS
#define REFRESH_RETRY_UNIT_SECONDS 3600L
#endif

/* Seconds from the given local time of day until the next refresh; always > 0
 * (exactly at the refresh time means a full day). */
long Refresh_SecondsUntilNext(int hour, int minute, int second);

/* Delay before retry number `step` (0 = the first retry). */
long Refresh_RetryDelaySeconds(int step);

/* How long to sleep, and why.
 *
 *   secondsToRefresh  seconds until the next midnight refresh, or -1 if the
 *                     clock is not set
 *   retryStep         -1 if the data is fine (nothing to retry), else the
 *                     retry number that is next due (0 = the first)
 *   *isRetry          set to 1 if the wake is a retry, 0 if it is the midnight
 *                     refresh
 *
 * A retry is used only if it falls before the midnight refresh, which then
 * takes over. Returns 0 if there is nothing to wake for (clock unknown and no
 * retry due); the caller then waits for a button press. */
long Refresh_NextWake(long secondsToRefresh, int retryStep, int *isRetry);

#endif
