#include "bin_icon.h"

#include "GUI_Paint.h"

static int maxInt(int a, int b) { return a > b ? a : b; }

/* Line weight scales with the icon so small icons stay crisp and large ones
 * do not look spindly. */
static DOT_PIXEL strokeFor(int w)
{
    if (w >= 100) return DOT_PIXEL_3X3;
    if (w >= 44)  return DOT_PIXEL_2X2;
    return DOT_PIXEL_1X1;
}

static void line(int x0, int y0, int x1, int y1, DOT_PIXEL stroke)
{
    Paint_DrawLine((UWORD)x0, (UWORD)y0, (UWORD)x1, (UWORD)y1, BLACK, stroke, LINE_STYLE_SOLID);
}

static void fillRect(int x0, int y0, int x1, int y1)
{
    for (int y = y0; y <= y1; y++) line(x0, y, x1, y, DOT_PIXEL_1X1);
}

static void dot(int cx, int cy, int r)
{
    Paint_DrawCircle((UWORD)cx, (UWORD)cy, (UWORD)r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

/* ------------------------------------------------------------ wheelie bin -- */

typedef struct {
    int topY, botY;         /* body extent */
    int topL, topR;         /* body left/right edge at topY */
    int botL, botR;         /* body left/right edge at botY */
} BodyShape;

static int edgeAt(int row, const BodyShape *b, int top, int bot)
{
    return top + (int)((long)(bot - top) * (row - b->topY) / maxInt(1, b->botY - b->topY));
}

static void drawWheelie(BinType bin, int x, int y, int w, int h)
{
    const DOT_PIXEL stroke = strokeFor(w);
    const int sw = (int)stroke;

    const int lidH   = maxInt(4, h * 13 / 100);
    const int gap    = maxInt(2, h / 40);
    const int wheelR = maxInt(2, h / 16);

    BodyShape b;
    b.topY = y + lidH + gap;
    b.botY = y + h - 1 - wheelR;
    b.topL = x + w * 4 / 100;   b.topR = x + w - 1 - w * 4 / 100;
    b.botL = x + w * 14 / 100;  b.botR = x + w - 1 - w * 14 / 100;

    /* Lid: always solid, slightly wider than the body. */
    fillRect(x, y, x + w - 1, y + lidH - 1);

    if (bin == BIN_BLACK) {
        for (int row = b.topY; row <= b.botY; row++) {
            line(edgeAt(row, &b, b.topL, b.botL), row, edgeAt(row, &b, b.topR, b.botR), row,
                 DOT_PIXEL_1X1);
        }
    } else {
        line(b.topL, b.topY, b.botL, b.botY, stroke);
        line(b.topR, b.topY, b.botR, b.botY, stroke);
        line(b.botL, b.botY, b.botR, b.botY, stroke);
        line(b.topL, b.topY, b.topR, b.topY, stroke);
    }

    /* Wheels */
    const int wheelY = y + h - 1 - wheelR;
    dot(x + w * 27 / 100, wheelY, wheelR);
    dot(x + w * 73 / 100, wheelY, wheelR);

    /* Marks on the outlined bins. */
    const int cx = x + w / 2;
    const int cy = (b.topY + b.botY) / 2;
    const int u  = maxInt(2, w / 6);

    if (bin == BIN_BLUE) {                          /* triangle */
        line(cx, cy - 2 * u, cx - 2 * u, cy + u + u / 2, stroke);
        line(cx, cy - 2 * u, cx + 2 * u, cy + u + u / 2, stroke);
        line(cx - 2 * u, cy + u + u / 2, cx + 2 * u, cy + u + u / 2, stroke);
    } else if (bin == BIN_RED) {                    /* solid band */
        for (int row = cy - u; row <= cy + u; row++) {
            line(edgeAt(row, &b, b.topL, b.botL) + sw + 2, row,
                 edgeAt(row, &b, b.topR, b.botR) - sw - 2, row, DOT_PIXEL_1X1);
        }
    } else if (bin == BIN_GARDEN) {                 /* sprig */
        line(cx, cy + 2 * u, cx, cy - u, stroke);
        dot(cx - u, cy,     maxInt(2, u));
        dot(cx + u, cy - u, maxInt(2, u));
    }
}

/* ------------------------------------------------------------- food caddy -- */

static void drawCaddy(int x, int y, int w, int h)
{
    const DOT_PIXEL stroke = strokeFor(w);

    const int handleH = h * 24 / 100;
    const int lidH    = maxInt(4, h * 12 / 100);
    const int gap     = maxInt(2, h / 40);

    /* Handle: a rectangular bail above the lid. */
    const int hx0 = x + w * 22 / 100, hx1 = x + w - 1 - w * 22 / 100;
    line(hx0, y + handleH, hx0, y + 1, stroke);
    line(hx1, y + handleH, hx1, y + 1, stroke);
    line(hx0, y + 1, hx1, y + 1, stroke);

    /* Lid */
    const int lidY = y + handleH;
    fillRect(x, lidY, x + w - 1, lidY + lidH - 1);

    /* Body: outlined, slightly tapered. */
    const int topY = lidY + lidH + gap, botY = y + h - 1;
    const int topL = x + w * 4 / 100,  topR = x + w - 1 - w * 4 / 100;
    const int botL = x + w * 10 / 100, botR = x + w - 1 - w * 10 / 100;
    line(topL, topY, botL, botY, stroke);
    line(topR, topY, botR, botY, stroke);
    line(botL, botY, botR, botY, stroke);
    line(topL, topY, topR, topY, stroke);

    /* Vent slits */
    const int slitTop = topY + (botY - topY) / 4;
    const int slitBot = topY + (botY - topY) * 3 / 4;
    for (int i = -1; i <= 1; i++) {
        const int sx = x + w / 2 + i * maxInt(3, w / 6);
        line(sx, slitTop, sx, slitBot, stroke);
    }
}

void BinIcon_Draw(BinType bin, int x, int y, int w, int h)
{
    if (bin == BIN_FOOD) drawCaddy(x, y, w, h);
    else                 drawWheelie(bin, x, y, w, h);
}
