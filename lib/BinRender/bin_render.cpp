#include "bin_render.h"

#include <stdio.h>
#include <string.h>

#include "bin_icon.h"
#include "bin_schedule.h"
#include "calendar_date.h"
#include "ui_text.h"

/* ---------------------------------------------------------------- layout -- */

#define SCREEN_W        800
#define SCREEN_H        480
#define MARGIN          10

#define HEADER_TOP      MARGIN
#define HEADER_RULE_Y   74
#define BODY_TOP        84
#define BODY_BOTTOM     (SCREEN_H - MARGIN)

/* Left: the hero panel for the next collection. Right: the days after it. */
#define HERO_X          MARGIN
#define HERO_W          470
#define HERO_PAD        14
#define HERO_BAR_H      28
#define HERO_ICONS_BOTTOM (BODY_BOTTOM - 10)

#define LIST_X          (HERO_X + HERO_W + 20)                  /* 500 */
#define LIST_W          (SCREEN_W - MARGIN - LIST_X)            /* 290 */
#define LIST_ROW_H      68
#define LIST_MAX_ROWS   5
#define LIST_ICON_W     27
#define LIST_ICON_H     36
#define LIST_ICON_GAP   4

/* ---------------------------------------------------------------- header -- */

static void drawHeader(const BinView *v)
{
    char buf[64];

    Paint_DrawString_EN(MARGIN, HEADER_TOP + 6, "Bin Collection", &Font24, BLACK, WHITE);

    const int dow = Cal_DayOfWeekMon0(v->todayYear, v->todayMonth, v->todayDay);
    snprintf(buf, sizeof(buf), "%s %d %s",
             Cal_WeekdayName(dow), v->todayDay, Cal_MonthName(v->todayMonth));
    Ui_DrawRight(SCREEN_W - MARGIN, HEADER_TOP + 4, buf, &Font16);

    /* A solid banner under the title, so untrustworthy data is impossible to miss. */
    if (v->warning && v->warning[0]) {
        char fit[64];
        Ui_FitText(fit, sizeof(fit), v->warning, &Font16, SCREEN_W - 2 * MARGIN - 20);
        const int bannerY = HEADER_TOP + 38;
        Paint_DrawRectangle(MARGIN, (UWORD)bannerY,
                            (UWORD)(MARGIN + Ui_TextWidth(fit, &Font16) + 20), (UWORD)(bannerY + 23),
                            BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawString_EN(MARGIN + 10, (UWORD)(bannerY + 4), fit, &Font16, WHITE, BLACK);
    }

    Paint_DrawLine(MARGIN, HEADER_RULE_Y, SCREEN_W - MARGIN, HEADER_RULE_Y,
                   BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
}

/* ------------------------------------------------------------------ hero -- */

/* Lists the bins in `mask` in display order, e.g. "Blue, Food, Garden". */
static void binNames(unsigned mask, char *buf, size_t len)
{
    buf[0] = '\0';
    for (int b = 0; b < BIN_TYPE_COUNT; b++) {
        if (!BinSchedule_HasBin(mask, (BinType)b)) continue;
        if (buf[0]) strncat(buf, ", ", len - strlen(buf) - 1);
        strncat(buf, Bin_Name((BinType)b), len - strlen(buf) - 1);
    }
}

static void drawHeroIcons(unsigned mask)
{
    const int n = BinSchedule_CountBins(mask);
    if (n <= 0) return;

    /* Fewer bins get bigger icons so the panel never looks half empty. */
    static const int kWidth[] = { 0, 104, 104, 100, 90, 78 };
    const int w = kWidth[n > 5 ? 5 : n];
    const int h = w * 4 / 3;

    const int avail = HERO_W - 2 * HERO_PAD;
    int gap = (n > 1) ? (avail - n * w) / (n - 1) : 0;
    if (gap > 28) gap = 28;

    /* Bottom-up: a small caption of what goes in the bin, its name, then the icon. */
    sFONT *nameFont = (w >= 100) ? &Font20 : &Font16;
    const int captionY = HERO_ICONS_BOTTOM - Font12.Height;
    const int nameY    = captionY - 4 - nameFont->Height;
    const int iconY    = nameY - 8 - h;

    const int total = n * w + (n - 1) * gap;
    int x = HERO_X + (HERO_W - total) / 2;

    for (int b = 0; b < BIN_TYPE_COUNT; b++) {
        if (!BinSchedule_HasBin(mask, (BinType)b)) continue;
        BinIcon_Draw((BinType)b, x, iconY, w, h);
        Ui_DrawCentred(x + w / 2, nameY, Bin_Name((BinType)b), nameFont);
        Ui_DrawCentred(x + w / 2, captionY, Bin_Contents((BinType)b), &Font12);
        x += w + gap;
    }
}

static void drawHero(const BinGroup *next)
{
    /* Card outline, with a solid title bar knocked out in white. */
    Paint_DrawRectangle(HERO_X, BODY_TOP, HERO_X + HERO_W, BODY_BOTTOM,
                        BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(HERO_X, BODY_TOP, HERO_X + HERO_W, BODY_TOP + HERO_BAR_H,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(HERO_X + HERO_PAD, BODY_TOP + 6, "NEXT COLLECTION",
                        &Font16, WHITE, BLACK);

    if (next == NULL) {
        Ui_DrawCentred(HERO_X + HERO_W / 2, BODY_TOP + 150, "No collections",
                       &Font24);
        Ui_DrawCentred(HERO_X + HERO_W / 2, BODY_TOP + 184, "scheduled",
                       &Font24);
        return;
    }

    int y, m, d;
    Cal_CivilFromDays(next->serial, &y, &m, &d);
    const int dow = Cal_DayOfWeekMon0(y, m, d);

    char label[16];
    Bin_HeroLabel(next->daysAway, dow, label, sizeof(label));

    const int textX = HERO_X + HERO_PAD;
    const int scale = Ui_FitScale(label, &Font24, 3, HERO_W - 2 * HERO_PAD);
    int cy = BODY_TOP + HERO_BAR_H + 14;
    Ui_DrawTextScaled(textX, cy, label, &Font24, scale);
    cy += Font24.Height * scale + 8;

    char buf[48];
    snprintf(buf, sizeof(buf), "%s %d %s", Cal_WeekdayName(dow), d, Cal_MonthName(m));
    Paint_DrawString_EN((UWORD)textX, (UWORD)cy, buf, &Font24, BLACK, WHITE);
    cy += Font24.Height + 10;

    /* Urgent days get a solid pill; later ones an outlined pill. */
    Bin_Advice(next->daysAway, buf, sizeof(buf));
    const int pillW = Ui_TextWidth(buf, &Font16) + 20;
    const int pillH = 26;
    if (next->daysAway <= 1) {
        Paint_DrawRectangle((UWORD)textX, (UWORD)cy, (UWORD)(textX + pillW), (UWORD)(cy + pillH),
                            BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawString_EN((UWORD)(textX + 10), (UWORD)(cy + 5), buf, &Font16, WHITE, BLACK);
    } else {
        Paint_DrawRectangle((UWORD)textX, (UWORD)cy, (UWORD)(textX + pillW), (UWORD)(cy + pillH),
                            BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
        Paint_DrawString_EN((UWORD)(textX + 10), (UWORD)(cy + 5), buf, &Font16, BLACK, WHITE);
    }

    drawHeroIcons(next->mask);
}

/* ------------------------------------------------------------------ list -- */

static void drawList(const BinGroup *groups, int count)
{
    Paint_DrawString_EN(LIST_X, BODY_TOP + 6, "FOLLOWING", &Font16, BLACK, WHITE);
    Paint_DrawLine(LIST_X, BODY_TOP + 28, SCREEN_W - MARGIN, BODY_TOP + 28,
                   BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    if (count <= 0) {
        Paint_DrawString_EN(LIST_X, BODY_TOP + 44, "Nothing further out.",
                            &Font16, BLACK, WHITE);
        return;
    }

    const int rows = count > LIST_MAX_ROWS ? LIST_MAX_ROWS : count;
    for (int i = 0; i < rows; i++) {
        const BinGroup *g = &groups[i];
        const int y0 = BODY_TOP + 38 + i * LIST_ROW_H;

        int y, m, d;
        Cal_CivilFromDays(g->serial, &y, &m, &d);
        const int dow = Cal_DayOfWeekMon0(y, m, d);

        char buf[48];
        char month[4];
        memcpy(month, Cal_MonthName(m), 3);
        month[3] = '\0';
        snprintf(buf, sizeof(buf), "%s %d %s", Cal_WeekdayShortName(dow), d, month);
        Paint_DrawString_EN(LIST_X, (UWORD)(y0 + 4), buf, &Font20, BLACK, WHITE);

        Bin_DaysAwayText(g->daysAway, buf, sizeof(buf));
        Paint_DrawString_EN(LIST_X, (UWORD)(y0 + 28), buf, &Font12, BLACK, WHITE);

        char names[64], fit[64];
        binNames(g->mask, names, sizeof(names));
        Ui_FitText(fit, sizeof(fit), names, &Font12, LIST_W);
        Paint_DrawString_EN(LIST_X, (UWORD)(y0 + 44), fit, &Font12, BLACK, WHITE);

        /* Mini icons, right aligned. */
        const int n = BinSchedule_CountBins(g->mask);
        int x = SCREEN_W - MARGIN - (n * LIST_ICON_W + (n - 1) * LIST_ICON_GAP);
        for (int b = 0; b < BIN_TYPE_COUNT; b++) {
            if (!BinSchedule_HasBin(g->mask, (BinType)b)) continue;
            BinIcon_Draw((BinType)b, x, y0 + 2, LIST_ICON_W, LIST_ICON_H);
            x += LIST_ICON_W + LIST_ICON_GAP;
        }

        if (i < rows - 1) {
            Paint_DrawLine(LIST_X, (UWORD)(y0 + LIST_ROW_H - 6),
                           SCREEN_W - MARGIN, (UWORD)(y0 + LIST_ROW_H - 6),
                           BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
        }
    }
}

/* ------------------------------------------------------------------ draw -- */

void BinRender_Draw(const BinView *v)
{
    Paint_Clear(WHITE);

    Paint_DrawRectangle(2, 2, SCREEN_W - 3, SCREEN_H - 3,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    drawHeader(v);

    /* One more than the list shows: the first group is the hero. */
    BinGroup groups[LIST_MAX_ROWS + 1];
    const int n = BinSchedule_Build(v, groups, LIST_MAX_ROWS + 1);

    drawHero(n > 0 ? &groups[0] : NULL);
    drawList(groups + 1, n > 0 ? n - 1 : 0);
}

void BinRender_DrawMessage(const char *title, const char *detail)
{
    Paint_Clear(WHITE);

    Paint_DrawRectangle(2, 2, SCREEN_W - 3, SCREEN_H - 3,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    Paint_DrawString_EN(MARGIN, HEADER_TOP + 6, "Bin Collection", &Font24, BLACK, WHITE);
    Paint_DrawLine(MARGIN, HEADER_RULE_Y, SCREEN_W - MARGIN, HEADER_RULE_Y,
                   BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawRectangle(HERO_X, BODY_TOP, SCREEN_W - MARGIN, BODY_BOTTOM,
                        BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    const int cx = SCREEN_W / 2;
    Ui_DrawCentred(cx, BODY_TOP + 130, title, &Font24);

    char fit[64];
    Ui_FitText(fit, sizeof(fit), detail, &Font16, SCREEN_W - 2 * (MARGIN + 30));
    Ui_DrawCentred(cx, BODY_TOP + 180, fit, &Font16);
    Ui_DrawCentred(cx, BODY_TOP + 210, "RESET, then BOOT, to try again.", &Font12);
}
