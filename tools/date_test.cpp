#include "calendar_render.h"
#include <stdio.h>
int fails=0;
static void chk(const char*what,long got,long want){ if(got!=want){printf("FAIL %s got %ld want %ld\n",what,got,want);fails++;} }
int main(){
  chk("dow 2026-08-24 (Mon=0)", Cal_DayOfWeekMon0(2026,8,24), 0);
  chk("dow 2026-08-01 (Sat=5)", Cal_DayOfWeekMon0(2026,8,1), 5);
  chk("dow 2000-01-01 (Sat=5)", Cal_DayOfWeekMon0(2000,1,1), 5);
  chk("dow 1970-01-01 (Thu=3)", Cal_DayOfWeekMon0(1970,1,1), 3);
  chk("dow 2024-02-29 (Thu=3)", Cal_DayOfWeekMon0(2024,2,29), 3);
  chk("dim 2024-02", Cal_DaysInMonth(2024,2), 29);
  chk("dim 2100-02", Cal_DaysInMonth(2100,2), 28);
  chk("dim 2000-02", Cal_DaysInMonth(2000,2), 29);
  chk("dim 2026-08", Cal_DaysInMonth(2026,8), 31);
  chk("serial 1970-01-01", Cal_DaysFromCivil(1970,1,1), 0);
  chk("serial 2026-08-24", Cal_DaysFromCivil(2026,8,24), 20689);
  // round trip across a wide range
  for(long s=-40000;s<40000;s+=7){int y,m,d;Cal_CivilFromDays(s,&y,&m,&d);
    if(Cal_DaysFromCivil(y,m,d)!=s){printf("FAIL roundtrip %ld -> %04d-%02d-%02d\n",s,y,m,d);fails++;break;}}
  // month rollover used by TOMORROW label
  {int y,m,d;Cal_CivilFromDays(Cal_DaysFromCivil(2026,12,31)+1,&y,&m,&d);
   if(y!=2027||m!=1||d!=1){printf("FAIL new year rollover %04d-%02d-%02d\n",y,m,d);fails++;}}
  printf(fails? "%d FAILURES\n":"all date checks passed\n", fails);
  return fails?1:0;
}
