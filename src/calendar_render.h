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

/* Date helpers, exposed because building dummy or live data needs them too. */
int  Cal_DaysInMonth(int year, int month);
int  Cal_DayOfWeekMon0(int year, int month, int day);  /* 0=Mon .. 6=Sun */
long Cal_DaysFromCivil(int year, int month, int day);  /* serial day number */
void Cal_CivilFromDays(long serial, int *year, int *month, int *day);

#endif
