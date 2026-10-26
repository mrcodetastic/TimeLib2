#include <unity.h>

#include "TimeConversions.hpp"

using namespace timelib2;

void test_epoch_zero() {
  TimeElements e;
  breakTime(0, e);

  TEST_ASSERT_EQUAL_UINT8(0, e.Second);
  TEST_ASSERT_EQUAL_UINT8(0, e.Minute);
  TEST_ASSERT_EQUAL_UINT8(0, e.Hour);
  TEST_ASSERT_EQUAL_UINT8(1, e.Day);
  TEST_ASSERT_EQUAL_UINT8(1, e.Month);
  TEST_ASSERT_EQUAL_UINT8(0, e.Year);
  TEST_ASSERT_EQUAL_UINT8(5, e.Wday);  // Thursday
  TEST_ASSERT_EQUAL_INT64(0, static_cast<int64_t>(makeTime(e)));
}

void test_known_date_2021() {
  // 2021-01-01T00:00:00Z
  const time_t epoch = 1609459200;
  TimeElements e;
  breakTime(epoch, e);

  TEST_ASSERT_EQUAL_UINT8(1, e.Day);
  TEST_ASSERT_EQUAL_UINT8(1, e.Month);
  TEST_ASSERT_EQUAL_UINT8(51, e.Year);
  TEST_ASSERT_EQUAL_UINT8(6, e.Wday);  // Friday
  TEST_ASSERT_EQUAL_INT64(epoch, static_cast<int64_t>(makeTime(e)));
}

void test_leap_day_2020() {
  // 2020-02-29T12:34:56Z
  const time_t epoch = 1582979696;
  TimeElements e;
  breakTime(epoch, e);

  TEST_ASSERT_EQUAL_UINT8(29, e.Day);
  TEST_ASSERT_EQUAL_UINT8(2, e.Month);
  TEST_ASSERT_EQUAL_UINT8(50, e.Year);
  TEST_ASSERT_EQUAL_UINT8(12, e.Hour);
  TEST_ASSERT_EQUAL_UINT8(34, e.Minute);
  TEST_ASSERT_EQUAL_UINT8(56, e.Second);
  TEST_ASSERT_EQUAL_UINT8(7, e.Wday);  // Saturday
  TEST_ASSERT_EQUAL_INT64(epoch, static_cast<int64_t>(makeTime(e)));
}

void test_2038_boundary() {
  // 2038-01-19T03:14:08Z, just past 2^31
  const time_t epoch = 2147483648LL;
  TimeElements e;
  breakTime(epoch, e);

  TEST_ASSERT_EQUAL_UINT8(19, e.Day);
  TEST_ASSERT_EQUAL_UINT8(1, e.Month);
  TEST_ASSERT_EQUAL_UINT8(68, e.Year);
  TEST_ASSERT_EQUAL_UINT8(3, e.Hour);
  TEST_ASSERT_EQUAL_INT64(epoch, static_cast<int64_t>(makeTime(e)));
}

void test_2100_is_not_a_leap_year() {
  // 2100-03-01T00:00:00Z
  const time_t epoch = 4107542400LL;
  TimeElements e;
  breakTime(epoch, e);

  TEST_ASSERT_EQUAL_UINT8(1, e.Day);
  TEST_ASSERT_EQUAL_UINT8(3, e.Month);
  TEST_ASSERT_EQUAL_UINT8(130, e.Year);
  TEST_ASSERT_EQUAL_INT64(epoch, static_cast<int64_t>(makeTime(e)));
}

void test_round_trip_samples() {
  const time_t samples[] = {
      0,          1,          59,         60,         3600,        86399,
      86400,      951782400,  1234567890, 2147483647, 2147483648LL, 4102444800LL};

  for (const time_t sample : samples) {
    TimeElements e;
    breakTime(sample, e);
    TEST_ASSERT_EQUAL_INT64(static_cast<int64_t>(sample),
                            static_cast<int64_t>(makeTime(e)));
  }
}

void test_date_strings() {
  TEST_ASSERT_EQUAL_STRING("January", monthStr(1));
  TEST_ASSERT_EQUAL_STRING("December", monthStr(12));
  TEST_ASSERT_EQUAL_STRING("", monthStr(0));
  TEST_ASSERT_EQUAL_STRING("", monthStr(13));

  TEST_ASSERT_EQUAL_STRING("Jan", monthShortStr(1));
  TEST_ASSERT_EQUAL_STRING("", monthShortStr(0));

  TEST_ASSERT_EQUAL_STRING("Sunday", dayStr(1));
  TEST_ASSERT_EQUAL_STRING("Saturday", dayStr(7));
  TEST_ASSERT_EQUAL_STRING("Err", dayStr(0));
  TEST_ASSERT_EQUAL_STRING("Err", dayStr(8));

  TEST_ASSERT_EQUAL_STRING("Sun", dayShortStr(1));
  TEST_ASSERT_EQUAL_STRING("Err", dayShortStr(9));
}

void test_constexpr_helpers() {
  TEST_ASSERT_EQUAL_INT64(300, static_cast<int64_t>(minutesToTime_t(5)));
  TEST_ASSERT_EQUAL_INT64(7200, static_cast<int64_t>(hoursToTime_t(2)));
  TEST_ASSERT_EQUAL_INT64(172800, static_cast<int64_t>(daysToTime_t(2)));
  TEST_ASSERT_EQUAL_INT64(1209600, static_cast<int64_t>(weeksToTime_t(2)));
  TEST_ASSERT_EQUAL_UINT8(5, dayOfWeek(0));  // 1970-01-01 is Thursday
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_epoch_zero);
  RUN_TEST(test_known_date_2021);
  RUN_TEST(test_leap_day_2020);
  RUN_TEST(test_2038_boundary);
  RUN_TEST(test_2100_is_not_a_leap_year);
  RUN_TEST(test_round_trip_samples);
  RUN_TEST(test_date_strings);
  RUN_TEST(test_constexpr_helpers);
  return UNITY_END();
}
