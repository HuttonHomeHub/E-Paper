#include "uk_clock.h"

#include <Arduino.h>
#include <esp_sntp.h>
#include <stdlib.h>

#include "refresh_schedule.h"

/* Anything earlier than this means the clock has not been set (it starts at
 * 1970): 2025-01-01 00:00:00 UTC. */
#define CLOCK_SET_AFTER 1735689600L

void UkClock_ApplyTimezone()
{
    setenv("TZ", UK_TZ, 1);
    tzset();
}

bool UkClock_SyncNtp(uint32_t timeoutMs)
{
    configTzTime(UK_TZ, "pool.ntp.org", "time.google.com");

    const unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) return true;
        delay(100);
    }
    return false;
}

bool UkClock_Now(struct tm *out)
{
    const time_t now = time(NULL);
    if (now < CLOCK_SET_AFTER) return false;
    localtime_r(&now, out);
    return true;
}

long UkClock_SecondsUntilRefresh()
{
    struct tm t;
    if (!UkClock_Now(&t)) return -1;
    return Refresh_SecondsUntilNext(t.tm_hour, t.tm_min, t.tm_sec);
}
