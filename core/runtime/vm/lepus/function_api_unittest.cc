// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <codecvt>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define RegisterFunctionAPI RegisterFunctionAPI_test

#include "core/runtime/vm/lepus/function_api.cc"

namespace lynx {
namespace lepus {

TEST(StringNumberConversionTest, StringToInt64Radix10) {
  static const struct {
    std::string input;
    int64_t output;
    bool success;
  } cases[] = {
      {"0", 0, true},
      {"42", 42, true},
      {"-2147483648", INT_MIN, true},
      {"2147483647", INT_MAX, true},
      {"-2147483649", INT64_C(-2147483649), true},
      {"-99999999999", INT64_C(-99999999999), true},
      {"2147483648", INT64_C(2147483648), true},
      {"99999999999", INT64_C(99999999999), true},
      {"9223372036854775807", std::numeric_limits<int64_t>::max(), true},
      {"-9223372036854775808", std::numeric_limits<int64_t>::min(), true},
      {"09", 9, true},
      {"-09", -9, true},
      {"", 0, false},
      {" 42", 42, true},
      {"42 ", 42, true},
      {"0x42", 0, true},
      {"\t\n\v\f\r 42", 42, true},
      {"blah42", 0, false},
      {"42blah", 42, true},
      {"blah42blah", 0, false},
      {"-273.15", -273, true},
      {"+98.6", 98, true},
      {"--123", 0, false},
      {"++123", 0, false},
      {"-+123", 0, false},
      {"+-123", 0, false},
      {"-", 0, false},
      {"-9223372036854775809", std::numeric_limits<int64_t>::min(), false},
      {"-99999999999999999999", std::numeric_limits<int64_t>::min(), false},
      {"9223372036854775808", std::numeric_limits<int64_t>::max(), false},
      {"99999999999999999999", std::numeric_limits<int64_t>::max(), false},
  };
  for (const auto& i : cases) {
    int64_t output = 0;
    EXPECT_EQ(i.success, ParseStringToInt(i.input, 10, output));
    EXPECT_EQ(i.output, output);
  }

  int64_t output = 0;
  const char input[] =
      "6\0"
      "6";
  EXPECT_EQ(ParseStringToInt(input, 10, output), true);
  EXPECT_EQ(output, 6);
}

TEST(StringNumberConversionsTest, HexStringToInt64) {
  static const struct {
    std::string input;
    int64_t output;
    bool success;
  } cases[] = {
      {"0", 0, true},
      {"42", 66, true},
      {"-42", -66, true},
      {"+42", 66, true},
      {"40acd88557b", INT64_C(4444444448123), true},
      {"7fffffff", INT_MAX, true},
      {"-80000000", INT_MIN, true},
      {"ffffffff", 0xffffffff, true},
      {"DeadBeef", 0xdeadbeef, true},
      {"0x42", 66, true},
      {"-0x42", -66, true},
      {"+0x42", 66, true},
      {"0x40acd88557b", INT64_C(4444444448123), true},
      {"0x7fffffff", INT_MAX, true},
      {"-0x80000000", INT_MIN, true},
      {"0xffffffff", 0xffffffff, true},
      {"0XDeadBeef", 0xdeadbeef, true},
      {"0x7fffffffffffffff", std::numeric_limits<int64_t>::max(), true},
      {"-0x8000000000000000", std::numeric_limits<int64_t>::min(), true},
      {"0x8000000000000000", std::numeric_limits<int64_t>::max(),
       false},  // Overflow test.
      {"-0x8000000000000001", std::numeric_limits<int64_t>::min(),
       false},  // Underflow test.
      {"0x0f", 15, true},
      {"0f", 15, true},
      {" 45", 0x45, true},
      {"\t\n\v\f\r 0x45", 0x45, true},
      {" 45", 0x45, true},
      {"45 ", 0x45, true},
      {"45:", 0x45, true},
      {"efgh", 0xef, true},
      {"0xefgh", 0xef, true},
      {"hgfe", 0, false},
      {"-", 0, false},
      {"", 0, false},
  };

  for (const auto& i : cases) {
    int64_t output = 0;
    EXPECT_EQ(i.success, ParseStringToInt(i.input, 16, output));
    EXPECT_EQ(i.output, output);
  }
  // One additional test to verify that conversion of numbers in strings with
  // embedded NUL characters.  The NUL and extra data after it should be
  // interpreted as junk after the number.
  const char input[] =
      "0xc0ffee\0"
      "9";
  std::string input_string(input, std::size(input) - 1);
  int64_t output;
  EXPECT_TRUE(ParseStringToInt(input_string, 16, output));
  EXPECT_EQ(0xc0ffee, output);
}

TEST(StringNumberConversionsTest, StringToDouble) {
  static const struct {
    std::string input;
    double output;
    bool success;
  } cases[] = {
      // Test different forms of zero.
      {"0", 0.0, true},
      {"+0", 0.0, true},
      {"-0", 0.0, true},
      {"0.0", 0.0, true},
      {"000000000000000000000000000000.0", 0.0, true},
      {"0.000000000000000000000000000", 0.0, true},

      // Test the answer.
      {"42", 42.0, true},
      {"-42", -42.0, true},

      // Test variances of an ordinary number.
      {"123.45", 123.45, true},
      {"-123.45", -123.45, true},
      {"+123.45", 123.45, true},

      // Test different forms of representation.
      {"2.99792458e8", 299792458.0, true},
      {"149597870.691E+3", 149597870691.0, true},
      {"6.", 6.0, true},

      // Test around the largest/smallest value that a double can represent.
      {"9e307", 9e307, true},
      {"1.7976e308", 1.7976e308, true},
      {"1.7977e308", HUGE_VAL, false},
      {"1.797693134862315807e+308", HUGE_VAL, true},
      // {"1.797693134862315808e+308", HUGE_VAL, false},
      {"9e308", HUGE_VAL, false},
      {"9e309", HUGE_VAL, false},
      {"9e999", HUGE_VAL, false},
      {"9e1999", HUGE_VAL, false},
      {"9e19999", HUGE_VAL, false},
      {"9e99999999999999999999", HUGE_VAL, false},
      {"-9e307", -9e307, true},
      {"-1.7976e308", -1.7976e308, true},
      {"-1.7977e308", -HUGE_VAL, false},
      {"-1.797693134862315807e+308", -HUGE_VAL, true},
      // {"-1.797693134862315808e+308", -HUGE_VAL, false},
      {"-9e308", -HUGE_VAL, false},
      {"-9e309", -HUGE_VAL, false},
      {"-9e999", -HUGE_VAL, false},
      {"-9e1999", -HUGE_VAL, false},
      {"-9e19999", -HUGE_VAL, false},
      {"-9e99999999999999999999", -HUGE_VAL, false},

      // Test more exponents.
      {"1e-2", 0.01, true},
      {"42 ", 42.0, true},
      {" 1e-2", 0.01, true},
      {"1e-2 ", 0.01, true},
      {"-1E-7", -0.0000001, true},
      {"01e02", 100, true},
      {"2.3e15", 2.3e15, true},
      {"100e-309", 100e-309, true},

      // Test some invalid cases.
      {"\t\n\v\f\r -123.45e2", -12345.0, true},
      {"+123 e4", 123.0, true},
      {"123e ", 123.0, true},
      {"123e", 123.0, true},
      {"10.5px", 10.5, true},
      {"11.5e2em", 1150, true},
      {" 2.99", 2.99, true},
      {"1e3.4", 1000.0, true},
      {"nothing", 0.0, false},
      {"-", 0.0, false},
      {"+", 0.0, false},
      {"", 0.0, false},

      // crbug.org/588726
      {"-0.0010000000000000000000000000000000000000001e-256",
       -1.0000000000000001e-259, true},
  };

  for (size_t i = 0; i < std::size(cases); ++i) {
    double output;
    EXPECT_EQ(cases[i].success, ParseStringToDouble(cases[i].input, output));
    EXPECT_DOUBLE_EQ(cases[i].output, output);
  }

  // One additional test to verify that conversion of numbers in strings with
  // embedded NUL characters.  The NUL and extra data after it should be
  // interpreted as junk after the number.
  const char input[] =
      "3.14\0"
      "159";
  std::string input_string(input, std::size(input) - 1);
  double output;
  EXPECT_TRUE(ParseStringToDouble(input_string, output));
  EXPECT_DOUBLE_EQ(3.14, output);
}

}  // namespace lepus
}  // namespace lynx
