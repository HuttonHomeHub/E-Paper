#include "refresh_schedule.h"

#define SECONDS_PER_DAY 86400L

long Refresh_SecondsUntilNext(int hour, int minute, int second)
{
    const long now    = (long)hour * 3600L + (long)minute * 60L + second;
    const long target = (long)REFRESH_HOUR * 3600L + (long)REFRESH_MINUTE * 60L;

    long wait = target - now;
    if (wait <= 0) wait += SECONDS_PER_DAY;
    return wait;
}
