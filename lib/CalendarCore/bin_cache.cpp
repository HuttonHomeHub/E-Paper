#include "bin_cache.h"

#include "calendar_date.h"

uint32_t BinCache_Pack(const BinCollection *c)
{
    return (((uint32_t)c->year * 100u + (uint32_t)c->month) * 100u + (uint32_t)c->day) * 10u
           + (uint32_t)c->bin;
}

int BinCache_Unpack(uint32_t packed, BinCollection *c)
{
    const int bin = (int)(packed % 10u);   packed /= 10u;
    const int day = (int)(packed % 100u);  packed /= 100u;
    const int month = (int)(packed % 100u); packed /= 100u;
    const int year = (int)packed;

    if (bin >= BIN_TYPE_COUNT) return 0;
    if (year < 2000 || year > 2999) return 0;
    if (month < 1 || month > 12) return 0;
    if (day < 1 || day > Cal_DaysInMonth(year, month)) return 0;

    c->year = year; c->month = month; c->day = day; c->bin = (BinType)bin;
    return 1;
}

uint32_t BinCache_DateKey(int year, int month, int day)
{
    return ((uint32_t)year * 100u + (uint32_t)month) * 100u + (uint32_t)day;
}
