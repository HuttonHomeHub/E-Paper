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

static void line(int x0, int y0, int x1, int y1, DOT_PIXEL stroke, UWORD colour = BLACK)
{
    Paint_DrawLine((UWORD)x0, (UWORD)y0, (UWORD)x1, (UWORD)y1, colour, stroke, LINE_STYLE_SOLID);
}

static void fillRect(int x0, int y0, int x1, int y1, UWORD colour = BLACK)
{
    for (int y = y0; y <= y1; y++) line(x0, y, x1, y, DOT_PIXEL_1X1, colour);
}

/* Filled triangle, by scanning its bounding box (icons are small, so this is cheap). */
static void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, UWORD colour = BLACK)
{
    int minX = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int maxX = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int minY = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int maxY = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            const long d1 = (long)(x - x1) * (y0 - y1) - (long)(x0 - x1) * (y - y1);
            const long d2 = (long)(x - x2) * (y1 - y2) - (long)(x1 - x2) * (y - y2);
            const long d3 = (long)(x - x0) * (y2 - y0) - (long)(x2 - x0) * (y - y0);
            const int neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            const int pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            if (!(neg && pos)) Paint_SetPixel((UWORD)x, (UWORD)y, colour);
        }
    }
}

static void dot(int cx, int cy, int r, UWORD colour = BLACK)
{
    Paint_DrawCircle((UWORD)cx, (UWORD)cy, (UWORD)r, colour, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

/* ------------------------------------------------------------ contents art -- */

/* Each of these draws a picture of what the bin takes, centred on (cx, cy),
 * about 2g wide and 2g tall. */

/* Blue: a bottle and a can. */
static void drawGlassAndCans(int cx, int cy, int g)
{
    const int bx = cx - g * 55 / 100;
    const int bw = maxInt(3, g * 70 / 100);
    const int nw = maxInt(2, bw / 2);
    const int bodyTop = cy - g * 20 / 100;
    fillRect(bx - bw / 2, bodyTop, bx + bw / 2, cy + g);            /* bottle body */
    fillRect(bx - nw / 2, cy - g, bx + nw / 2, bodyTop);            /* bottle neck */

    const int kx = cx + g * 55 / 100;
    const int kw = maxInt(3, g * 80 / 100);
    const int canTop = cy - g * 45 / 100;
    fillRect(kx - kw / 2, canTop, kx + kw / 2, cy + g);             /* can */
    if (g >= 10) {                                                  /* rim, once big enough */
        const int rim = canTop + maxInt(2, g / 5);
        line(kx - kw / 2, rim, kx + kw / 2, rim, DOT_PIXEL_1X1, WHITE);
    }
}

/* Black: a rubbish sack, knocked out in white on the solid body. A round bag
 * pinched at the top, with a black tie band and two chunky gathered ears. */
static void drawSack(int cx, int cy, int g)
{
    const int r       = maxInt(3, g * 80 / 100);
    const int by      = cy + g * 35 / 100;                          /* sack centre */
    const int neckBot = by - r * 75 / 100;                          /* where the bag pinches */
    const int nw      = maxInt(1, r / 4);                           /* neck half width */
    const int t       = maxInt(3, g * 40 / 100);                    /* ear size */
    const int neckTop = neckBot - maxInt(2, g / 4);

    dot(cx, by, r, WHITE);                                          /* sack body */
    fillRect(cx - nw, neckTop, cx + nw, neckBot + 1, WHITE);        /* pinched neck */

    /* Ears: two triangles fanning up and out from the top of the neck. */
    fillTriangle(cx, neckTop + 1, cx - 2 * t, neckTop - t, cx - t / 2, neckTop - t - t / 2, WHITE);
    fillTriangle(cx, neckTop + 1, cx + 2 * t, neckTop - t, cx + t / 2, neckTop - t - t / 2, WHITE);

    /* The tie: a black band across the neck. */
    const int tie = (g >= 14) ? 1 : 0;
    fillRect(cx - nw - 1, neckBot - tie, cx + nw + 1, neckBot, BLACK);
}

/* Red: a cardboard box with a flap and a strip of tape. */
static void drawBox(int cx, int cy, int g, DOT_PIXEL stroke)
{
    const int top = cy - g * 70 / 100;
    const int bot = cy + g * 70 / 100;
    const int flap = top + g * 45 / 100;
    const int tape = maxInt(1, g / 6);

    Paint_DrawRectangle((UWORD)(cx - g), (UWORD)top, (UWORD)(cx + g), (UWORD)bot,
                        BLACK, stroke, DRAW_FILL_EMPTY);
    line(cx - g, flap, cx + g, flap, DOT_PIXEL_1X1);
    fillRect(cx - tape, top, cx + tape, flap);
}

/* Garden: a stem with two leaves. */
static void drawSprig(int cx, int cy, int g, DOT_PIXEL stroke)
{
    const int leaf = maxInt(2, g / 2);
    line(cx, cy + g, cx, cy - g / 2, stroke);
    dot(cx - leaf, cy,          leaf);
    dot(cx + leaf, cy - g / 2,  leaf);
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

    /* What goes in it, centred in the body. */
    const int cx = x + w / 2;
    const int cy = (b.topY + b.botY) / 2;
    const int g  = maxInt(3, w * 30 / 100);

    const DOT_PIXEL boxStroke = (stroke == DOT_PIXEL_1X1) ? DOT_PIXEL_1X1 : DOT_PIXEL_2X2;

    switch (bin) {
        case BIN_BLUE:   drawGlassAndCans(cx, cy, g);      break;
        case BIN_BLACK:  drawSack(cx, cy, g);              break;
        case BIN_RED:    drawBox(cx, cy, g, boxStroke);   break;
        case BIN_GARDEN: drawSprig(cx, cy, g, stroke);     break;
        default: break;
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
