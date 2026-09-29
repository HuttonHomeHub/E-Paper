/**
 * Draws the bin collection screen into the currently selected GUI_Paint canvas.
 *
 * Pure drawing - no Arduino, no network, no clock. Call Paint_NewImage() and
 * Paint_SelectImage() first; this clears the canvas and draws over it.
 */
#ifndef BIN_RENDER_H
#define BIN_RENDER_H

#include "bin_data.h"

void BinRender_Draw(const BinView *view);

/* Shown instead of the schedule when the data could not be loaded, so a failure
 * is visible rather than leaving stale or made-up dates on the panel. */
void BinRender_DrawMessage(const char *title, const char *detail);

#endif
