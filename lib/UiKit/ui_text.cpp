#include "ui_text.h"

#include <stdio.h>
#include <string.h>

int Ui_TextWidth(const char *s, sFONT *font)
{
    return (int)strlen(s) * font->Width;
}

void Ui_FitText(char *dst, size_t dstLen, const char *src, sFONT *font, int maxPx)
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

void Ui_DrawRight(int rightX, int y, const char *s, sFONT *font, UWORD fg, UWORD bg)
{
    Paint_DrawString_EN((UWORD)(rightX - Ui_TextWidth(s, font)), (UWORD)y, s, font, fg, bg);
}

void Ui_DrawCentred(int cx, int y, const char *s, sFONT *font, UWORD fg, UWORD bg)
{
    Paint_DrawString_EN((UWORD)(cx - Ui_TextWidth(s, font) / 2), (UWORD)y, s, font, fg, bg);
}

int Ui_TextWidthScaled(const char *s, sFONT *font, int scale)
{
    return Ui_TextWidth(s, font) * scale;
}

int Ui_FitScale(const char *s, sFONT *font, int maxScale, int maxPx)
{
    int scale = maxScale;
    while (scale > 1 && Ui_TextWidthScaled(s, font, scale) > maxPx) scale--;
    return scale < 1 ? 1 : scale;
}

void Ui_DrawTextScaled(int x, int y, const char *s, sFONT *font, int scale, UWORD fg)
{
    const int bytesPerRow = font->Width / 8 + (font->Width % 8 ? 1 : 0);

    for (; *s; s++, x += font->Width * scale) {
        if (*s < ' ' || *s > '~') continue;

        const uint8_t *glyph = &font->table[(*s - ' ') * font->Height * bytesPerRow];
        for (int row = 0; row < font->Height; row++) {
            for (int col = 0; col < font->Width; col++) {
                const uint8_t bits = glyph[row * bytesPerRow + col / 8];
                if (!(bits & (0x80 >> (col % 8)))) continue;

                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        Paint_SetPixel((UWORD)(x + col * scale + dx),
                                       (UWORD)(y + row * scale + dy), fg);
                    }
                }
            }
        }
    }
}
