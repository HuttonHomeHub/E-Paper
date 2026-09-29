/**
 * UK local time on the ESP32: timezone setup, NTP sync, and "how long until the
 * next midnight refresh". Needs the Arduino ESP32 core.
 *
 * The system clock keeps running through deep sleep, but the timezone setting
 * does not - call UkClock_ApplyTimezone() at the start of every boot.
 */
#ifndef UK_CLOCK_H
#define UK_CLOCK_H

#include <stdint.h>
#include <time.h>

/* GMT, with BST from the last Sunday of March to the last Sunday of October. */
#define UK_TZ "GMT0BST,M3.5.0/1,M10.5.0"

void UkClock_ApplyTimezone();

/* Asks NTP for the time; the WiFi must already be up. Returns true once the
 * clock has been set from the network, false if it timed out. */
bool UkClock_SyncNtp(uint32_t timeoutMs);

/* Local time now. False if the clock has never been set (it reads 1970 after
 * a power-up or RESET until NTP has run). */
bool UkClock_Now(struct tm *out);

/* Seconds until the next daily refresh (see refresh_schedule.h), or -1 if the
 * clock is not set. */
long UkClock_SecondsUntilRefresh();

#endif
