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

#endif
