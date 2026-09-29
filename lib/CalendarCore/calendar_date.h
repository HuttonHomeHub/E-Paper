/**
 * Civil-calendar arithmetic used by the calendar screen.
 *
 * Pure C++ with no Arduino, display or network dependencies, so it is unit
 * tested on the host (`pio test -e native`).
 */
#ifndef CALENDAR_DATE_H
#define CALENDAR_DATE_H

int  Cal_DaysInMonth(int year, int month);
int  Cal_DayOfWeekMon0(int year, int month, int day);  /* 0=Mon .. 6=Sun */
long Cal_DaysFromCivil(int year, int month, int day);  /* serial day, 1970-01-01 == 0 */
void Cal_CivilFromDays(long serial, int *year, int *month, int *day);

#endif
