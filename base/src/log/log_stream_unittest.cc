// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/log_stream.h"

#include <algorithm>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace logging {
template <typename INT>
struct NumberContrastString {
  INT number;
  std::string expected;
  std::string unsigned_expected;
};

template <typename INT>
std::string ConvertToString(const INT& number) {
  LogStream output;
  output << number;
  return output.str();
}

template <typename T>
void SmartPointerToHexString() {
  srand(time(nullptr));
  int32_t array_length = rand() % 100 + 10;
  std::vector<T> tests;
  for (int32_t i = 0; i < array_length; ++i) {
    tests.emplace_back(T(new int32_t(i)));
  }

  for (const auto& test : tests) {
    std::ostringstream output;
    output << test;
    std::string output_str = output.str();
    std::transform(output_str.begin() + 2, output_str.end(),
                   output_str.begin() + 2, ::toupper);

    std::string sub_output = output_str.substr(2);
    std::string sub_logstream_output = ConvertToString(test).substr(2);
    size_t position1 = sub_output.find_first_not_of("0");
    size_t position2 = sub_logstream_output.find_first_not_of("0");

    EXPECT_EQ(sub_logstream_output.substr(position2),
              sub_output.substr(position1));
  }
}

class SelfType {
  double value_;

 public:
  explicit SelfType(double value) : value_(value) {}
  inline friend LogStream& operator<<(LogStream& output,
                                      const SelfType& input) {
    output << input.value_;
    return output;
  }
};

TEST(LogStreamTest, BoolToString) {
  static const struct {
    bool input;
    std::string output;
  } cases[] = {{true, "true"}, {false, "false"}};

  for (const auto& test : cases) {
    EXPECT_EQ(ConvertToString<bool>(test.input), test.output);
  }
}

TEST(LogStreamTest, NumberToString) {
  static const NumberContrastString<int8_t> int8_tests[] = {
      {0, "0", "0"},
      {-1, "-1", "255"},
      {std::numeric_limits<int8_t>::max(), "127", "127"},
      {std::numeric_limits<int8_t>::min(), "-128", "128"},
  };

  static const NumberContrastString<int16_t> int16_tests[] = {
      {0, "0", "0"},
      {-1, "-1", "65535"},
      {std::numeric_limits<int16_t>::max(), "32767", "32767"},
      {std::numeric_limits<int16_t>::min(), "-32768", "32768"},
  };

  static const NumberContrastString<int> int_tests[] = {
      {0, "0", "0"},
      {-1, "-1", "4294967295"},
      {std::numeric_limits<int>::max(), "2147483647", "2147483647"},
      {std::numeric_limits<int>::min(), "-2147483648", "2147483648"},
  };
  static const NumberContrastString<int64_t> int64_tests[] = {
      {0, "0", "0"},
      {-1, "-1", "18446744073709551615"},
      {
          std::numeric_limits<int64_t>::max(),
          "9223372036854775807",
          "9223372036854775807",
      },
      {std::numeric_limits<int64_t>::min(), "-9223372036854775808",
       "9223372036854775808"},
  };

  for (const auto& test : int8_tests) {
    EXPECT_EQ(ConvertToString(test.number), test.expected);
    EXPECT_EQ(ConvertToString(static_cast<uint8_t>(test.number)),
              test.unsigned_expected);
  }

  for (const auto& test : int16_tests) {
    EXPECT_EQ(ConvertToString(test.number), test.expected);
    EXPECT_EQ(ConvertToString(static_cast<uint16_t>(test.number)),
              test.unsigned_expected);
  }

  for (const auto& test : int_tests) {
    EXPECT_EQ(ConvertToString(test.number), test.expected);
    EXPECT_EQ(ConvertToString(static_cast<unsigned>(test.number)),
              test.unsigned_expected);
  }

  for (const auto& test : int64_tests) {
    EXPECT_EQ(ConvertToString(test.number), test.expected);
    EXPECT_EQ(ConvertToString(static_cast<uint64_t>(test.number)),
              test.unsigned_expected);
  }
}

TEST(LogStreamTest, Uint64ToString) {
  static const struct {
    uint64_t input;
    std::string output;
  } cases[] = {
      {0, "0"},
      {42, "42"},
      {INT_MAX, "2147483647"},
      {std::numeric_limits<uint64_t>::max(), "18446744073709551615"},
  };

  for (const auto& test : cases) {
    EXPECT_EQ(ConvertToString(test.input), test.output);
  }
}

TEST(LogStreamTest, Size_TToString) {
  static const struct {
    size_t input;
    std::string output;
  } cases[] = {
      {0, "0"},
      {9, "9"},
      {42, "42"},
      {INT_MAX, "2147483647"},
      {2147483648U, "2147483648"},
  };

  for (const auto& test : cases) {
    EXPECT_EQ(ConvertToString(test.input), test.output);
  }
}

TEST(LogStreamTest, floatToString) {
  static const struct {
    float input;
    std::string expected;
  } cases[] = {
      {0.0, "0"},
      {0.5, "0.5"},
      {1.25, "1.25"},
      {3.1415926, "3.14159"},
      {2.123456789, "2.12346"},
      {2.12345678912345, "2.12346"},
      {1.123e-14, "1.123e-14"},
      {1e-17, "1e-17"},
      {1.33545e+09, "1.33545e+09"},
  };

  for (const auto& test : cases) {
    EXPECT_EQ(ConvertToString(test.input), test.expected);
  }
}

TEST(LogStreamTest, DoubleToString) {
  static const struct {
    double input;
    std::string expected;
  } cases[] = {
      {0.0, "0.0"},     {0.5, "0.5"},
      {1.25, "1.25"},   {1.123e-14, "1.123e-14"},
      {1e-17, "1e-17"}, {1.33545e+009, "1335450000.0"},
  };

  for (const auto& test : cases) {
    EXPECT_EQ(ConvertToString(test.input), test.expected);
  }

  // The following two values were seen in crashes in the wild.
  const char input_bytes[8] = {0, 0, 0, 0, '\xee', '\x6d', '\x73', '\x42'};
  double input = 0;
  memcpy(&input, input_bytes, sizeof(input_bytes) / sizeof(input_bytes[0]));
  EXPECT_EQ("1335179083776.0", ConvertToString(input));
  const char input_bytes2[8] = {0,      0,      0,      '\xa0',
                                '\xda', '\x6c', '\x73', '\x42'};
  input = 0;
  memcpy(&input, input_bytes2, sizeof(input_bytes2) / sizeof(input_bytes2[0]));
  EXPECT_EQ("1334890332160.0", ConvertToString(input));
}

TEST(LogStreamTest, AddressToHexString) {
  static const char* cases[] = {"0", "42", "-42", "7fffffff", "0XDeadBeef"};

  for (const auto& test : cases) {
    std::ostringstream output;
    output << &test;
    std::string output_str = output.str();
    std::transform(output_str.begin() + 2, output_str.end(),
                   output_str.begin() + 2, ::toupper);

    std::string sub_output = output_str.substr(2);
    std::string sub_logstream_output = ConvertToString(&test).substr(2);
    size_t position1 = sub_output.find_first_not_of("0");
    size_t position2 = sub_logstream_output.find_first_not_of("0");

    EXPECT_EQ(sub_logstream_output.substr(position2),
              sub_output.substr(position1));
  }
}

TEST(LogStreamTest, SharedPtrToHexString) {
  SmartPointerToHexString<std::shared_ptr<int32_t>>();
}

TEST(LogStreamTest, UniquePtrToHexString) {
  SmartPointerToHexString<std::unique_ptr<int32_t>>();
}

TEST(LogStreamTest, AtomicToHexString) {
  {
    // int
    int32_t value = 1024;
    std::atomic<int32_t> input(value);
    LogStream output;
    output << input;
    EXPECT_EQ(output.str(), "1024");
  }

  {
    // double
    double value = 3.124;
    std::atomic<double> input(value);
    LogStream output;
    output << input;
    EXPECT_EQ(output.str(), "3.124");
  }

  {
    // char
    char value = 'a';
    std::atomic<char> input(value);
    LogStream output;
    output << input;
    EXPECT_EQ(output.str(), "a");
  }

  // UDT which is trivially copyable
  {
    double value = 3.1415926;
    SelfType temp(value);
    std::atomic<SelfType> input(temp);
    LogStream output;
    output << input;
    EXPECT_EQ(output.str(), "3.1415926");
  }
}

#if defined(OS_WIN)
TEST(LogStreamTest, WstringToString) {
  static const struct {
    std::wstring input;
    std::string output;
  } cases[] = {
      {L"0", "0"},
      {L"42", "42"},
      {L"2147483647", "2147483647"},
      {L"Hello World", "Hello World"},
  };

  for (const auto& test : cases) {
    EXPECT_EQ(ConvertToString(test.input), test.output);
  }
}

TEST(LogStreamTest, WCharToString) {
  static const struct {
    wchar_t input;
    std::string output;
  } cases[] = {{L'1', "1"},   {L'a', "a"},   {L'b', "b"},
               {L'c', "c"},   {L'\r', "\r"}, {L'\t', "\t"},
               {L'\n', "\n"}, {L'汉', "汉"}, {0x6C49, "汉"}};

  for (const auto& test : cases) {
    EXPECT_EQ(ConvertToString(test.input), test.output);
  }
}
#endif

TEST(LogStreamTest, NullCharToString) {
  const std::string target_string =
      "When input is nullptr, truncate output stream";
  const std::string need_truncated = "need to be truncated";
  char* null_char_ptr = nullptr;
  LogStream output;
  output << target_string << null_char_ptr << need_truncated;
  EXPECT_EQ(output.str(), target_string);
}

TEST(LogStreamTest, StringViewToString) {
  const std::string target_string = "convert string_view to string";
  const std::string_view target_view(target_string);
  LogStream output;
  output << target_view;
  EXPECT_EQ(output.str(), target_string);
}

TEST(LogStreamTest, LogStreamBase) {
  const char* target_string = "Welcome to the world of lynx";
  LogStream output;
  output << target_string;
  EXPECT_TRUE(output.Buffer().Length() != 0);

  output.Reset();
  EXPECT_TRUE(output.Buffer().Length() == 0);

  output << "Today is " << 2022 << "-" << 11 << "-" << 2;
  EXPECT_TRUE(output.Buffer().Length() != 0);
  output.Clear();
  EXPECT_TRUE(output.Buffer().Length() == 0);

  // test overload std::ostingstream
  std::ostringstream std_os;
  std_os << target_string;
  output << std_os;
  EXPECT_EQ(output.str(), std_os.str());
  output.Clear();
}

}  // namespace logging
}  // namespace base
}  // namespace lynx
