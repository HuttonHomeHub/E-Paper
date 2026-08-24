#include "calendar_render.h"
#include "GUI_Paint.h"
#include "fonts.h"

#include <string.h>
#include <stdio.h>

/* ---------------------------------------------------------------- layout -- */

#define SCREEN_W        800
#define SCREEN_H        480
#define MARGIN          10

#define HEADER_TOP      MARGIN
#define HEADER_RULE_Y   74
#define BODY_TOP        84
#define BODY_BOTTOM     (SCREEN_H - MARGIN)

/* Left: month grid. Right: agenda. Split by a vertical rule. */
#define GRID_X          MARGIN
#define GRID_COL_W      65
#define GRID_COLS       7
#define GRID_W          (GRID_COL_W * GRID_COLS)      /* 455 */
#define GRID_HDR_H      24
#define GRID_ROWS       6
#define GRID_ROW_H      60
#define GRID_BODY_TOP   (BODY_TOP + GRID_HDR_H)

#define VRULE_X         (GRID_X + GRID_W + 14)        /* 479 */
#define AGENDA_X        (VRULE_X + 14)                /* 493 */
#define AGENDA_W        (SCREEN_W - MARGIN - AGENDA_X)/* 297 */

#define AGENDA_ROW_H    36
#define AGENDA_DAY_H    26

static const char *kMonthNames[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
static const char *kWeekdayShort[] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
static const char *kWeekdayLong[]  = { "Monday", "Tuesday", "Wednesday", "Thursday",
                                       "Friday", "Saturday", "Sunday" };

/* ----------------------------------------------------------------- dates -- */

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

/* ------------------------------------------------------------ text utils -- */

/* Copies src into dst, truncating with "..." so it fits maxPx at this font. */
static void fitText(char *dst, size_t dstLen, const char *src, sFONT *font, int maxPx)
{
    const int maxChars = maxPx / font->Width;
    const int srcLen   = (int)strlen(src);

    if (maxChars <= 0) { dst[0] = '\0'; return; }

    if (srcLen <= maxChars) {
        snprintf(dst, dstLen, "%s", src);
        return;
    }
    int keep = maxChars - 3;
    if (keep < 1) keep = maxChars;          /* too narrow for an ellipsis */
    if (keep > (int)dstLen - 4) keep = (int)dstLen - 4;
    memcpy(dst, src, (size_t)keep);
    dst[keep] = '\0';
    if (keep == maxChars - 3) strncat(dst, "...", dstLen - strlen(dst) - 1);
}

static int textWidth(const char *s, sFONT *font)
{
    return (int)strlen(s) * font->Width;
}

static void drawRight(int rightX, int y, const char *s, sFONT *font)
{
    Paint_DrawString_EN((UWORD)(rightX - textWidth(s, font)), (UWORD)y, s, font, BLACK, WHITE);
}

static void drawCentred(int cx, int y, const char *s, sFONT *font, UWORD fg, UWORD bg)
{
    Paint_DrawString_EN((UWORD)(cx - textWidth(s, font) / 2), (UWORD)y, s, font, fg, bg);
}

/* ---------------------------------------------------------------- header -- */

static void drawHeader(const CalView *v)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "%s %d", kMonthNames[v->todayMonth - 1], v->todayYear);
    Paint_DrawString_EN(MARGIN, HEADER_TOP + 6, buf, &Font24, BLACK, WHITE);

    const int dow = Cal_DayOfWeekMon0(v->todayYear, v->todayMonth, v->todayDay);
    snprintf(buf, sizeof(buf), "%s %d %s",
             kWeekdayLong[dow], v->todayDay, kMonthNames[v->todayMonth - 1]);
    drawRight(SCREEN_W - MARGIN, HEADER_TOP + 4, buf, &Font16);

    if (v->statusLine && v->statusLine[0]) {
        drawRight(SCREEN_W - MARGIN, HEADER_TOP + 26, v->statusLine, &Font12);
    }

    Paint_DrawLine(MARGIN, HEADER_RULE_Y, SCREEN_W - MARGIN, HEADER_RULE_Y,
                   BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

/* ------------------------------------------------------------ month grid -- */

static int eventsOnDay(const CalView *v, int y, int m, int d)
{
    int n = 0;
    for (int i = 0; i < v->eventCount; i++) {
        const CalEvent *e = &v->events[i];
        if (e->year == y && e->month == m && e->day == d) n++;
    }
    return n;
}

static void drawMonthGrid(const CalView *v)
{
    /* Weekday headings */
    for (int c = 0; c < GRID_COLS; c++) {
        const int cx = GRID_X + c * GRID_COL_W + GRID_COL_W / 2;
        drawCentred(cx, BODY_TOP + 4, kWeekdayShort[c], &Font12, BLACK, WHITE);
    }
    Paint_DrawLine(GRID_X, GRID_BODY_TOP - 4, GRID_X + GRID_W, GRID_BODY_TOP - 4,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    const int firstDow  = Cal_DayOfWeekMon0(v->todayYear, v->todayMonth, 1);
    const int daysThis  = Cal_DaysInMonth(v->todayYear, v->todayMonth);

    int prevYear = v->todayYear, prevMonth = v->todayMonth - 1;
    if (prevMonth < 1) { prevMonth = 12; prevYear--; }
    const int daysPrev = Cal_DaysInMonth(prevYear, prevMonth);

    for (int cell = 0; cell < GRID_ROWS * GRID_COLS; cell++) {
        const int row = cell / GRID_COLS;
        const int col = cell % GRID_COLS;
        const int x   = GRID_X + col * GRID_COL_W;
        const int y   = GRID_BODY_TOP + row * GRID_ROW_H;

        const int dayNum = cell - firstDow + 1;
        char label[8];

        if (dayNum < 1) {
            /* Trailing days of the previous month, set smaller so they recede. */
            snprintf(label, sizeof(label), "%d", daysPrev + dayNum);
            drawCentred(x + GRID_COL_W / 2, y + 14, label, &Font12, BLACK, WHITE);
            continue;
        }
        if (dayNum > daysThis) {
            snprintf(label, sizeof(label), "%d", dayNum - daysThis);
            drawCentred(x + GRID_COL_W / 2, y + 14, label, &Font12, BLACK, WHITE);
            continue;
        }

        const int isToday = (dayNum == v->todayDay);
        snprintf(label, sizeof(label), "%d", dayNum);

        if (isToday) {
            /* Solid block behind today's number, number knocked out in white. */
            Paint_DrawRectangle((UWORD)(x + 8), (UWORD)(y + 2),
                                (UWORD)(x + GRID_COL_W - 8), (UWORD)(y + 30),
                                BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            drawCentred(x + GRID_COL_W / 2, y + 5, label, &Font20, WHITE, BLACK);
        } else {
            drawCentred(x + GRID_COL_W / 2, y + 5, label, &Font20, BLACK, WHITE);
        }

        /* A dot per event, up to three, so busy days read at a glance. */
        int n = eventsOnDay(v, v->todayYear, v->todayMonth, dayNum);
        if (n > 3) n = 3;
        if (n > 0) {
            const int dotY  = y + 42;
            const int gap   = 12;
            const int startX = x + GRID_COL_W / 2 - ((n - 1) * gap) / 2;
            for (int i = 0; i < n; i++) {
                Paint_DrawCircle((UWORD)(startX + i * gap), (UWORD)dotY, 3,
                                 BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            }
        }
    }
}

/* ---------------------------------------------------------------- agenda -- */

static void formatTime(const CalEvent *e, char *buf, size_t len)
{
    if (e->startMin == CAL_ALL_DAY) {
        snprintf(buf, len, "All day");
        return;
    }
    if (e->endMin >= 0) {
        snprintf(buf, len, "%02d:%02d - %02d:%02d",
                 e->startMin / 60, e->startMin % 60, e->endMin / 60, e->endMin % 60);
    } else {
        snprintf(buf, len, "%02d:%02d", e->startMin / 60, e->startMin % 60);
    }
}

static void dayLabel(const CalView *v, long serial, char *buf, size_t len)
{
    const long todaySerial = Cal_DaysFromCivil(v->todayYear, v->todayMonth, v->todayDay);
    int yy, mm, dd;
    Cal_CivilFromDays(serial, &yy, &mm, &dd);

    if (serial == todaySerial)      { snprintf(buf, len, "TODAY"); return; }
    if (serial == todaySerial + 1)  { snprintf(buf, len, "TOMORROW"); return; }

    const int dow = Cal_DayOfWeekMon0(yy, mm, dd);
    char month[4];
    memcpy(month, kMonthNames[mm - 1], 3);
    month[3] = '\0';
    snprintf(buf, len, "%s %d %s", kWeekdayShort[dow], dd, month);
}

static void drawAgenda(const CalView *v)
{
    Paint_DrawString_EN(AGENDA_X, BODY_TOP, "UPCOMING", &Font16, BLACK, WHITE);
    Paint_DrawLine(AGENDA_X, BODY_TOP + 20, SCREEN_W - MARGIN, BODY_TOP + 20,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    const long todaySerial = Cal_DaysFromCivil(v->todayYear, v->todayMonth, v->todayDay);

    int  y = BODY_TOP + 30;
    long lastSerial = -999999L;
    int  shown = 0;
    char buf[CAL_TITLE_LEN + 16];

    for (int i = 0; i < v->eventCount; i++) {
        const CalEvent *e = &v->events[i];
        const long serial = Cal_DaysFromCivil(e->year, e->month, e->day);
        if (serial < todaySerial) continue;                 /* already past */

        const int needsHeader = (serial != lastSerial);
        const int needed = (needsHeader ? AGENDA_DAY_H : 0) + AGENDA_ROW_H;
        if (y + needed > BODY_BOTTOM) {
            const int remaining = v->eventCount - i;
            if (remaining > 0 && y + 18 <= BODY_BOTTOM) {
                snprintf(buf, sizeof(buf), "+ %d more", remaining);
                Paint_DrawString_EN(AGENDA_X, y, buf, &Font12, BLACK, WHITE);
            }
            break;
        }

        if (needsHeader) {
            dayLabel(v, serial, buf, sizeof(buf));
            Paint_DrawString_EN(AGENDA_X, y + 4, buf, &Font12, BLACK, WHITE);
            Paint_DrawLine(AGENDA_X, y + 19, SCREEN_W - MARGIN, y + 19,
                           BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
            y += AGENDA_DAY_H;
            lastSerial = serial;
        }

        formatTime(e, buf, sizeof(buf));
        Paint_DrawString_EN(AGENDA_X, y, buf, &Font12, BLACK, WHITE);

        char title[CAL_TITLE_LEN];
        fitText(title, sizeof(title), e->title, &Font16, AGENDA_W);
        Paint_DrawString_EN(AGENDA_X, y + 14, title, &Font16, BLACK, WHITE);

        y += AGENDA_ROW_H;
        shown++;
    }

    if (shown == 0) {
        Paint_DrawString_EN(AGENDA_X, BODY_TOP + 40, "Nothing coming up.",
                            &Font16, BLACK, WHITE);
    }
}

/* ------------------------------------------------------------------ draw -- */

void CalendarRender_Draw(const CalView *v)
{
    Paint_Clear(WHITE);

    Paint_DrawRectangle(2, 2, SCREEN_W - 3, SCREEN_H - 3,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    drawHeader(v);
    drawMonthGrid(v);

    Paint_DrawLine(VRULE_X, BODY_TOP, VRULE_X, BODY_BOTTOM,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    drawAgenda(v);
}
