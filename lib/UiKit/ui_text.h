/**
 * Text helpers shared by the screen renderers, built on GUI_Paint and its
 * fixed bitmap fonts. Every call draws onto the currently selected canvas.
 */
#ifndef UI_TEXT_H
#define UI_TEXT_H

#include <stddef.h>

#include "GUI_Paint.h"
#include "fonts.h"

int  Ui_TextWidth(const char *s, sFONT *font);

/* Copies src into dst, truncating with "..." so it fits maxPx at this font. */
void Ui_FitText(char *dst, size_t dstLen, const char *src, sFONT *font, int maxPx);

void Ui_DrawRight(int rightX, int y, const char *s, sFONT *font,
                  UWORD fg = BLACK, UWORD bg = WHITE);
void Ui_DrawCentred(int cx, int y, const char *s, sFONT *font,
                    UWORD fg = BLACK, UWORD bg = WHITE);

/* Draws s with every font pixel enlarged to scale x scale, for headlines the
 * built-in fonts are too small for. Only set pixels are painted, so it sits
 * cleanly on whatever is already drawn. */
void Ui_DrawTextScaled(int x, int y, const char *s, sFONT *font, int scale, UWORD fg = BLACK);
int  Ui_TextWidthScaled(const char *s, sFONT *font, int scale);

/* Largest scale in [1, maxScale] at which s still fits maxPx (minimum 1). */
int  Ui_FitScale(const char *s, sFONT *font, int maxScale, int maxPx);

#endif
