/**
 * Black-and-white bin pictograms drawn with GUI_Paint primitives, so they scale
 * to any size without bitmap assets.
 *
 * Wheelie bins share one silhouette (solid lid, tapered body, wheels) and are
 * told apart by their body: Black is solid, the others are outlined with a mark
 * (Blue: triangle, Red: band, Garden: sprig). Food is a caddy with a handle and
 * vent slits. The panel has no colour, so the bin's name is always drawn
 * alongside by the caller.
 *
 * The icon fills the box (x, y) .. (x + w - 1, y + h - 1); a w:h of about 3:4
 * looks right for the wheelie bins.
 */
#ifndef BIN_ICON_H
#define BIN_ICON_H

#include "bin_data.h"

void BinIcon_Draw(BinType bin, int x, int y, int w, int h);

#endif
