/**
 * Draws the calendar screen into the currently selected GUI_Paint canvas.
 *
 * Pure drawing - no Arduino, no network, no clock. Call Paint_NewImage() and
 * Paint_SelectImage() first; this clears the canvas and draws over it.
 */
#ifndef CALENDAR_RENDER_H
#define CALENDAR_RENDER_H

#include "calendar_data.h"

void CalendarRender_Draw(const CalView *view);

#endif
