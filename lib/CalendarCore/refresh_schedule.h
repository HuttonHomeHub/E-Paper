/**
 * When the display should next redraw by itself.
 *
 * The pages show dates, never the time of day, so the only moment they go out
 * of date is midnight ("TOMORROW" becomes "TODAY"). The board therefore sleeps
 * until just after midnight and redraws. Pure arithmetic, unit tested.
 */
#ifndef REFRESH_SCHEDULE_H
#define REFRESH_SCHEDULE_H

/* Local time of the daily refresh. A few minutes past midnight rather than on
 * it, so a wake that comes slightly early (the ESP32's sleep timer is not a
 * precision clock) still lands on the new day. */
#define REFRESH_HOUR    0
#define REFRESH_MINUTE  5

/* Seconds from the given local time of day until the next refresh; always > 0
 * (exactly at the refresh time means a full day). */
long Refresh_SecondsUntilNext(int hour, int minute, int second);

#endif
