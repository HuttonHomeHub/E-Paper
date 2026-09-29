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

long Refresh_RetryDelaySeconds(int step)
{
    static const int kUnits[] = { 1, 2, 4 };
    if (step < 0) step = 0;
    const int units = (step < 3) ? kUnits[step] : 1;    /* then every unit */
    return (long)units * REFRESH_RETRY_UNIT_SECONDS;
}

long Refresh_NextWake(long secondsToRefresh, int retryStep, int *isRetry)
{
    int retry = 0;
    long wake = 0;

    if (retryStep < 0) {
        wake = secondsToRefresh > 0 ? secondsToRefresh : 0;
    } else {
        const long delay = Refresh_RetryDelaySeconds(retryStep);
        if (secondsToRefresh > 0 && secondsToRefresh <= delay) {
            wake = secondsToRefresh;                    /* midnight comes first */
        } else {
            wake = delay;
            retry = 1;
        }
    }

    if (isRetry) *isRetry = retry;
    return wake;
}
