// Host tests for clock_face.h. Run: pio test -e native
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unity.h>

#include "clock_face.h"

void setUp() {}
void tearDown() {}

// data.h stores epoch + offset in the system clock; loop() reads it back
// with gmtime_r. Do the same here.
static struct tm localFromBridge(int64_t epoch, int32_t tzOffset) {
  time_t local = (time_t)(epoch + tzOffset);
  struct tm t;
  gmtime_r(&local, &t);
  return t;
}

// Regression. M5StickS3 has no RTC chip, so M5.Rtc.getDate() never wrote
// its output and every field kept the M5Unified default of -1. The old
// drawClock() then read DOW[255] and crashed in snprintf.
static void test_unread_rtc_fields_do_not_index_past_tables() {
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_sec = t.tm_min = t.tm_hour = -1;
  t.tm_mday = t.tm_mon = t.tm_wday = -1;
  ClockFaceText out;
  clockFaceText(t, out);
  TEST_ASSERT_EQUAL_STRING("??? -1", out.date);
  TEST_ASSERT_EQUAL_STRING("??? ??? -1", out.wdate);
}

// Bridge payload {"time":[1790860919,-14400]}: 2026-10-01 13:21:59 UTC,
// shown at UTC-4.
static void test_bridge_time_sync_example() {
  struct tm t = localFromBridge(1790860919, -14400);
  ClockFaceText out;
  clockFaceText(t, out);
  TEST_ASSERT_EQUAL_STRING("09:21", out.hm);
  TEST_ASSERT_EQUAL_STRING(":59", out.ss);
  TEST_ASSERT_EQUAL_STRING("59", out.sec);
  TEST_ASSERT_EQUAL_STRING("Oct 01", out.date);
  TEST_ASSERT_EQUAL_STRING("Thu Oct 01", out.wdate);
}

static uint64_t nextRandom(uint64_t& s) {  // xorshift64
  s ^= s << 13; s ^= s >> 7; s ^= s << 17;
  return s;
}

static void assertMatchesStrftime(const struct tm& t, const char* what) {
  ClockFaceText out;
  clockFaceText(t, out);
  char ref[32];
  strftime(ref, sizeof(ref), "%H:%M", &t);    TEST_ASSERT_EQUAL_STRING_MESSAGE(ref, out.hm, what);
  strftime(ref, sizeof(ref), ":%S", &t);      TEST_ASSERT_EQUAL_STRING_MESSAGE(ref, out.ss, what);
  strftime(ref, sizeof(ref), "%S", &t);       TEST_ASSERT_EQUAL_STRING_MESSAGE(ref, out.sec, what);
  strftime(ref, sizeof(ref), "%b %d", &t);    TEST_ASSERT_EQUAL_STRING_MESSAGE(ref, out.date, what);
  strftime(ref, sizeof(ref), "%a %b %d", &t); TEST_ASSERT_EQUAL_STRING_MESSAGE(ref, out.wdate, what);
}

// Property: for every epoch and offset the bridge can send (the firmware
// time_t is 32 bits), the text equals strftime in the C locale.
static void test_any_bridge_time_matches_strftime() {
  uint64_t s = 0x9E3779B97F4A7C15ull;
  for (int i = 0; i < 200000; i++) {
    int64_t epoch = (int32_t)nextRandom(s);
    int32_t tz = (int32_t)(nextRandom(s) % (2 * 14 * 3600 + 1)) - 14 * 3600;
    if (epoch + tz < INT32_MIN || epoch + tz > INT32_MAX) continue;
    char what[48];
    snprintf(what, sizeof(what), "epoch=%lld tz=%d", (long long)epoch, (int)tz);
    assertMatchesStrftime(localFromBridge(epoch, tz), what);
  }
}

// Property: any field values give text that fits its buffer, and names
// come only from the tables or "???". ASan reports a read past a table.
static void test_any_field_values_stay_in_bounds() {
  static const int EDGE[] = { INT_MIN, -129, -128, -1, 0, 6, 7, 11, 12, 31, 59, 60, 255, 256, INT_MAX };
  const int nEdge = sizeof(EDGE) / sizeof(EDGE[0]);
  uint64_t s = 0xD1B54A32D192ED03ull;
  for (int i = 0; i < 200000; i++) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    int* f[] = { &t.tm_sec, &t.tm_min, &t.tm_hour, &t.tm_mday, &t.tm_mon, &t.tm_wday };
    for (int* p : f) {
      uint64_t r = nextRandom(s);
      *p = (r & 1) ? EDGE[(r >> 1) % nEdge] : (int)(r >> 32);
    }
    ClockFaceText out;
    clockFaceText(t, out);
    TEST_ASSERT_TRUE(memchr(out.hm, 0, sizeof(out.hm)) != nullptr);
    TEST_ASSERT_TRUE(memchr(out.ss, 0, sizeof(out.ss)) != nullptr);
    TEST_ASSERT_TRUE(memchr(out.sec, 0, sizeof(out.sec)) != nullptr);
    TEST_ASSERT_TRUE(memchr(out.date, 0, sizeof(out.date)) != nullptr);
    TEST_ASSERT_TRUE(memchr(out.wdate, 0, sizeof(out.wdate)) != nullptr);
    bool monOk = (unsigned)t.tm_mon < 12;
    bool dowOk = (unsigned)t.tm_wday < 7;
    TEST_ASSERT_EQUAL(monOk, strncmp(out.date, "???", 3) != 0);
    TEST_ASSERT_EQUAL(dowOk, strncmp(out.wdate, "???", 3) != 0);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_unread_rtc_fields_do_not_index_past_tables);
  RUN_TEST(test_bridge_time_sync_example);
  RUN_TEST(test_any_bridge_time_matches_strftime);
  RUN_TEST(test_any_field_values_stay_in_bounds);
  return UNITY_END();
}
