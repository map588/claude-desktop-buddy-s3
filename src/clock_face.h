#pragma once
#include <stdio.h>
#include <time.h>

// Text for the charging clock face. clockFaceText() fills it from local
// broken-down time. It accepts any field values: a month or weekday out of
// range gives "???". It reads the name tables only after a range check, so
// a bad time source cannot read past them (an unread RTC once gave weekday
// -1, and the read past the table crashed the stick).
struct ClockFaceText {
  char hm[6];      // "HH:MM"       portrait and landscape
  char ss[4];      // ":SS"         portrait
  char sec[3];     // "SS"          landscape
  char date[8];    // "Mon DD"      portrait
  char wdate[12];  // "Dow Mon DD"  landscape
};

inline void clockFaceText(const struct tm& t, ClockFaceText& out) {
  static const char* const MON[] = {
    "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
  };
  static const char* const DOW[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  const char* mon = (unsigned)t.tm_mon  < 12 ? MON[t.tm_mon]  : "???";
  const char* dow = (unsigned)t.tm_wday < 7  ? DOW[t.tm_wday] : "???";
  snprintf(out.hm,    sizeof(out.hm),    "%02d:%02d", t.tm_hour, t.tm_min);
  snprintf(out.ss,    sizeof(out.ss),    ":%02d", t.tm_sec);
  snprintf(out.sec,   sizeof(out.sec),   "%02d", t.tm_sec);
  snprintf(out.date,  sizeof(out.date),  "%s %02d", mon, t.tm_mday);
  snprintf(out.wdate, sizeof(out.wdate), "%s %s %02d", dow, mon, t.tm_mday);
}
