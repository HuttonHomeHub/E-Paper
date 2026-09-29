#include "bin_schedule.h"

#include <stdio.h>

#include "calendar_date.h"

int BinSchedule_Build(const BinView *view, BinGroup *out, int maxGroups)
{
    if (view == NULL || out == NULL || maxGroups <= 0) return 0;

    const long today = Cal_DaysFromCivil(view->todayYear, view->todayMonth, view->todayDay);
    int n = 0;

    for (int i = 0; i < view->collectionCount; i++) {
        const BinCollection *c = &view->collections[i];
        if ((int)c->bin < 0 || (int)c->bin >= BIN_TYPE_COUNT) continue;

        const long serial = Cal_DaysFromCivil(c->year, c->month, c->day);
        if (serial < today) continue;

        /* Find the group for this day, or the slot that keeps dates sorted. */
        int pos = 0;
        while (pos < n && out[pos].serial < serial) pos++;

        if (pos < n && out[pos].serial == serial) {
            out[pos].mask |= 1u << c->bin;
            continue;
        }
        if (pos >= maxGroups) continue;             /* later than everything kept */

        if (n < maxGroups) n++;                     /* else the last group falls off */
        for (int j = n - 1; j > pos; j--) out[j] = out[j - 1];
        out[pos].serial   = serial;
        out[pos].daysAway = (int)(serial - today);
        out[pos].mask     = 1u << c->bin;
    }
    return n;
}

int BinSchedule_CountBins(unsigned mask)
{
    int n = 0;
    for (int b = 0; b < BIN_TYPE_COUNT; b++) {
        if (mask & (1u << b)) n++;
    }
    return n;
}

int BinSchedule_HasBin(unsigned mask, BinType bin)
{
    return (int)bin >= 0 && (int)bin < BIN_TYPE_COUNT && (mask & (1u << bin)) != 0;
}

const char *Bin_Name(BinType bin)
{
    switch (bin) {
        case BIN_BLUE:   return "Blue";
        case BIN_BLACK:  return "Black";
        case BIN_RED:    return "Red";
        case BIN_FOOD:   return "Food";
        case BIN_GARDEN: return "Garden";
        default:         return "";
    }
}

void Bin_HeroLabel(int daysAway, int dowMon0, char *buf, size_t len)
{
    if (daysAway <= 0) { snprintf(buf, len, "TODAY"); return; }
    if (daysAway == 1) { snprintf(buf, len, "TOMORROW"); return; }
    /* A weekday name a week or more out would read as "this coming one". */
    if (daysAway >= 7) { snprintf(buf, len, "IN %d DAYS", daysAway); return; }

    const char *name = Cal_WeekdayName(dowMon0);
    size_t i = 0;
    for (; name[i] != '\0' && i + 1 < len; i++) {
        const char ch = name[i];
        buf[i] = (ch >= 'a' && ch <= 'z') ? (char)(ch - 'a' + 'A') : ch;
    }
    if (len > 0) buf[i] = '\0';
}

void Bin_Advice(int daysAway, char *buf, size_t len)
{
    if (daysAway <= 0)      snprintf(buf, len, "Collection day");
    else if (daysAway == 1) snprintf(buf, len, "Put bins out tonight");
    else if (daysAway >= 7) snprintf(buf, len, "Not this week");
    else                    snprintf(buf, len, "In %d days", daysAway);
}

void Bin_DaysAwayText(int daysAway, char *buf, size_t len)
{
    if (daysAway <= 0)      snprintf(buf, len, "Today");
    else if (daysAway == 1) snprintf(buf, len, "Tomorrow");
    else                    snprintf(buf, len, "In %d days", daysAway);
}
