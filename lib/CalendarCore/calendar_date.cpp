#include "calendar_date.h"

int Cal_DaysInMonth(int year, int month)
{
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12) return 30;
    if (month == 2) {
        int leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return leap ? 29 : 28;
    }
    return days[month - 1];
}

/* Howard Hinnant's days_from_civil: serial day number, 1970-01-01 == 0. */
long Cal_DaysFromCivil(int y, int m, int d)
{
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153u * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097L + (long)doe - 719468L;
}

void Cal_CivilFromDays(long z, int *year, int *month, int *day)
{
    z += 719468L;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long y = (long)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned d = doy - (153 * mp + 2) / 5 + 1;
    const unsigned m = mp + (mp < 10 ? 3 : -9);
    *year  = (int)(y + (m <= 2));
    *month = (int)m;
    *day   = (int)d;
}

int Cal_DayOfWeekMon0(int year, int month, int day)
{
    /* 1970-01-01 was a Thursday (index 3 with Monday as 0). */
    long s = Cal_DaysFromCivil(year, month, day);
    int dow = (int)((s + 3) % 7);
    if (dow < 0) dow += 7;
    return dow;
}
