// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/string/string_utils.h"

#include <string>
#include <type_traits>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(StringUtils, ConvertParenthesesStringToVector) {
  int idx = 1;
  std::string s1("(1,2,3,4)");
  std::vector<std::string> ret;

  ConvertParenthesesStringToVector(s1, ret, ',');
  EXPECT_EQ(ret.size(), static_cast<size_t>(4));
  for (auto& it : ret) {
    EXPECT_EQ(it, std::to_string(idx++));
  }
  ret.clear();

  idx = 1;
  s1 = "( 1,    2,    3,  4 )";
  ConvertParenthesesStringToVector(s1, ret, ',');
  EXPECT_EQ(ret.size(), static_cast<size_t>(4));
  for (auto& it : ret) {
    EXPECT_EQ(it, std::to_string(idx++));
  }
  ret.clear();

  idx = 1;
  s1 = "(@1@@2@@@3@@@@4)";
  ConvertParenthesesStringToVector(s1, ret, '@');
  EXPECT_EQ(ret.size(), static_cast<size_t>(4));
  for (auto& it : ret) {
    EXPECT_EQ(it, std::to_string(idx++));
  }
  ret.clear();

  idx = 1;
  s1 = "(@1@@2@@@3@@@@4)";
  ConvertParenthesesStringToVector(s1, ret, ',');
  EXPECT_EQ(ret.size(), static_cast<size_t>(1));
  ret.clear();

  idx = 1;
  s1 = "@1@@2@@@3@@@@4";
  ConvertParenthesesStringToVector(s1, ret, '@');
  EXPECT_EQ(ret.size(), static_cast<size_t>(0));
  ret.clear();
}

TEST(StringNumberConvertTest, SplitStringIgnoreBracket) {
  int idx = 1;
  std::string s1("1,2,3,4");
  std::vector<std::string> ret;

  ret = SplitStringIgnoreBracket(s1, ',');
  EXPECT_EQ(ret.size(), static_cast<size_t>(4));
  for (auto& it : ret) {
    EXPECT_EQ(it, std::to_string(idx++));
  }
  ret.clear();

  s1 = "1,(2,3),4";
  ret = SplitStringIgnoreBracket(s1, ',');
  EXPECT_EQ(ret.size(), static_cast<size_t>(3));
  EXPECT_EQ(ret[0], "1");
  EXPECT_EQ(ret[1], "(2,3)");
  EXPECT_EQ(ret[2], "4");
  ret.clear();

  s1 = "(1,2,3,4)";
  ret = SplitStringIgnoreBracket(s1, ',');
  EXPECT_EQ(ret.size(), static_cast<size_t>(1));
  EXPECT_EQ(ret[0], "(1,2,3,4)");
  ret.clear();
}

TEST(StringNumberConvertTest, ReplaceMultiSpaceWithOne) {
  std::string s1("1,   2,  3, 4");
  ReplaceMultiSpaceWithOne(s1);
  EXPECT_EQ("1, 2, 3, 4", s1);

  s1 = "1,2,3,4";
  ReplaceMultiSpaceWithOne(s1);
  EXPECT_EQ("1,2,3,4", s1);
}

TEST(StringNumberConvertTest, ReplaceEscapeCharacterWithLiteralString) {
  std::vector<std::pair<std::string, std::string>> input = {
      {"", ""},
      {"\\n", "\\n"},
      {"\"a \nb\"", "\"a \\nb\""},
      {"( xxx ? \"a\" : \"b\")", "( xxx ? \"a\" : \"b\")"},
      {"( xxx ? \n                \"a\" : \n                \"b\")",
       "( xxx ? \n                \"a\" : \n                \"b\")"},
      {"( xxx ? \n    a : \n    b )", "( xxx ? \n    a : \n    b )"},
      {"\"a\"", "\"a\""},
      {"\"a \nb\"", "\"a \\nb\""},
      {"\"\\\"a\\\"\"", "\"\\\"a\\\"\""},
      {"\"\\\"a\\\" \\n \\\"b\\\"\"", "\"\\\"a\\\" \\n \\\"b\\\"\""},
      {"\"\\\"a \nb\\\"\"", "\"\\\"a \\nb\\\"\""},
      {"\"\\\"a \nb\\\"\"", "\"\\\"a \\nb\\\"\""},
      {"( xxx ? \"a\" : \"b\")+\"\\\"\"", "( xxx ? \"a\" : \"b\")+\"\\\"\""},
      {"( xxx ? \n                \"a\" : \n                \"b\")+\"\\\">\"",
       "( xxx ? \n                \"a\" : \n                \"b\")+\"\\\">\""},
      {"( xxx ? \n    a : \n    b )", "( xxx ? \n    a : \n    b )"},
  };
  for (auto& pair : input) {
    ReplaceEscapeCharacterWithLiteralString(pair.first);
    ASSERT_EQ(pair.first, pair.second);
  }
}

TEST(CamelCaseToDashCaseTest, CamelCaseToDashCase) {
  std::vector<std::pair<std::string, std::string>> inputs = {
      {"", ""},
      {"123", "123"},
      {"aaaa", "aaaa"},
      {"fontSize", "font-size"},
      {"backgroundColor", "background-color"},
      {"listCrossAxisGap", "list-cross-axis-gap"},
  };

  for (auto& pair : inputs) {
    EXPECT_EQ(CamelCaseToDashCase(pair.first), pair.second);
  }
}

TEST(SplitStringByCharsOrderlyTest, SplitStringByCharsOrderly) {
  std::string input;
  base::Vector<std::string> expected;

  input = "color: white; font-size: 100";
  auto result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"color", " white", " font-size", " 100"};
  ASSERT_EQ(result, expected);

  input = "color:white; font-size:100";
  result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"color", "white", " font-size", "100"};
  ASSERT_EQ(result, expected);

  input = "color:white;:;width:100";
  result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"color", "white", "", "", "width", "100"};
  ASSERT_EQ(result, expected);

  input = "font-family:'white';width:100";
  result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"font-family", "'white'", "width", "100"};
  ASSERT_EQ(result, expected);

  input = "background-image: url('https://xxxx.jpg');";
  result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"background-image", " url('https://xxxx.jpg')"};
  ASSERT_EQ(result, expected);

  input = "background-image: url(https://xxxx.jpg);";
  result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"background-image", " url(https://xxxx.jpg)"};
  ASSERT_EQ(result, expected);

  input = "background-image: url(\"https://xxxx.jpg\");";
  result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"background-image", " url(\"https://xxxx.jpg\")"};
  ASSERT_EQ(result, expected);

  input = "background-image: {x:xx}";
  result = SplitStringByCharsOrderly<':', ';'>(input);
  expected = {"background-image", " {x:xx}"};
  ASSERT_EQ(result, expected);
}

TEST(U8StringToU16Test, U8StringToU16) {
  std::vector<std::string> input;
  std::vector<std::u16string> expected;

  // err1:The string is out of the range of UTF-8!
  std::vector<uint8_t> test1 = {std::uint8_t(0b10000000)};
  std::string err1(test1.begin(), test1.end());
  // err2:The string is out of the range of UTF-8!
  std::vector<uint8_t> test2 = {std::uint8_t(0b11000000),
                                std::uint8_t(0b11101110)};
  std::string err2(test2.begin(), test2.end());
  // err3:An error occurred in some UTF-8 strings!
  std::vector<uint8_t> test3 = {std::uint8_t(0b11100000),
                                std::uint8_t(0b11000010),
                                std::uint8_t(0b10101110)};
  std::string err3(test3.begin(), test3.end());
  // err4:The string is out of the range of UTF-8!
  std::vector<uint8_t> test4 = {
      std::uint8_t(0b11111000), std::uint8_t(0b10101110),
      std::uint8_t(0b10101110), std::uint8_t(0b10101110)};
  std::string err4(test4.begin(), test4.end());
  // err5:An error occurred in some UTF-8 strings
  std::vector<uint8_t> test5 = {std::uint8_t(0b11011000),
                                std::uint8_t(0b00101110)};
  std::string err5(test5.begin(), test5.end());
  // err6:The UTF-8 string is missing bytes!
  std::vector<uint8_t> test6 = {std::uint8_t(0b11101000),
                                std::uint8_t(0b10101110)};
  std::string err6(test6.begin(), test6.end());
  // err7:he UTF-8 string is missing bytes!
  std::vector<uint8_t> test7 = {
      std::uint8_t(0b11010000), std::uint8_t(0b10101110),
      std::uint8_t(0b11101000), std::uint8_t(0b10101110)};
  std::string err7(test7.begin(), test7.end());

  input = {u8".?\"`~-_=+} 、,.<｜｜》〉？'/]>{.[$¥%^",
           u8"hello,WORLD!",
           u8"世界,你好！",
           u8"",
           u8"\uC10F",
           u8"      ",
           u8"\u07FF",
           u8"\uFFFF\u079E",
           u8"\U0010EEEE",
           u8"\u0000",
           u8"\U00004E23\U0001F601\U00001EB5",
           u8"\U00001EB7",
           u8"\U00000152",
           u8"\U000020A7",
           u8"\U0001F606",
           err1,
           err2,
           err3,
           err4,
           err5,
           err6,
           err7};
  expected = {u".?\"`~-_=+} 、,.<｜｜》〉？'/]>{.[$¥%^",
              u"hello,WORLD!",
              u"世界,你好！",
              u"",
              u"\U0000C10F",
              u"      ",
              u"\U000007FF",
              u"\U0000FFFF\U0000079E",
              u"\U0010EEEE",
              u"",
              u"\U00004E23\U0001F601\U00001EB5",
              u"\U00001EB7",
              u"\U00000152",
              u"\U000020A7",
              u"\U0001F606",
              u"",
              u"",
              u"",
              u"",
              u"",
              u"",
              u""};
  for (size_t i = 0; i < input.size(); ++i) {
    std::u16string result = U8StringToU16(input[i]);
    ASSERT_EQ(result, expected[i]);
  }
}

TEST(U16StringToU8Test, U16StringToU8) {
  std::vector<std::u16string> input;
  std::vector<std::string> expected;

  // err1:The string is out of the range of UTF-16!
  std::vector<uint16_t> test1 = {std::uint16_t(0x0020), std::uint16_t(0xFFFF)};
  std::u16string err1(test1.begin(), test1.end());
  // err2:The string is out of the range of UTF-16!
  std::vector<uint16_t> test2 = {std::uint16_t(0x0010), std::uint16_t(0xFFFF),
                                 std::uint16_t(0x0020), std::uint16_t(0xFFFF)};
  std::u16string err2(test2.begin(), test2.end());

  input = {u".?\"`~-_=+} 、,.<｜｜》〉？'/]>{.[$¥%^",
           u"hello,WORLD!",
           u"世界,你好！",
           u"",
           u"\uC10F",
           u"      ",
           u"\U000007FF",
           u"\U0000FFFF\U0000079E",
           u"\u006E",
           u"\u06EE",
           u"\u08FF\u06FF",
           u"\u08FF\U0010EEEE\U0001FFFF",
           u"\u0000",
           u"\uFFFF",
           u"\U00100000",
           u"\U0010FFFF",
           u"\U00004E23\U0001F601\U00001EB5",
           u"\U00001EB7",
           u"\U00000152",
           u"\U000020A7",
           u"\U0001F606"};
  expected = {u8".?\"`~-_=+} 、,.<｜｜》〉？'/]>{.[$¥%^",
              u8"hello,WORLD!",
              u8"世界,你好！",
              u8"",
              u8"\uC10F",
              u8"      ",
              u8"\u07FF",
              u8"\uFFFF\u079E",
              u8"\u006E",
              u8"\u06EE",
              u8"\u08FF\u06FF",
              u8"\u08FF\U0010EEEE\U0001FFFF",
              u8"\u0000",
              u8"\uFFFF",
              u8"\U00100000",
              u8"\U0010FFFF",
              u8"\U00004E23\U0001F601\U00001EB5",
              u8"\U00001EB7",
              u8"\U00000152",
              u8"\U000020A7",
              u8"\U0001F606"};

  for (size_t i = 0; i < input.size(); ++i) {
    std::string result = U16StringToU8(input[i]);
    ASSERT_EQ(result, expected[i]);
  }
}

TEST(U8StringToU16Test, Utf16ToUtf16Empty) {
  EXPECT_EQ(U8StringToU16(""), u"");
}

TEST(U8StringToU16Test, Utf8ToUtf16Ascii) {
  EXPECT_EQ(U8StringToU16("abc123"), u"abc123");
}

TEST(U8StringToU16Test, Utf8ToUtf16Unicode) {
  EXPECT_EQ(U8StringToU16("\xe2\x98\x83"), u"\x2603");
}

TEST(U16StringToU8Test, Utf16ToUtf8Empty) { EXPECT_EQ(U16StringToU8(u""), ""); }

TEST(U16StringToU8Test, Utf16ToUtf8Ascii) {
  EXPECT_EQ(U16StringToU8(u"abc123"), "abc123");
}

TEST(U16StringToU8Test, Utf16ToUtf8Unicode) {
  EXPECT_EQ(U16StringToU8(u"\x2603"), "\xe2\x98\x83");
}

TEST(FormatStringTest, FormatString) {
  int* ptr = new int(10);
  constexpr char str_format[] =
      "the string is %s, the num is %d, the char is %c, the pointer is %p";
  std::stringstream ss;
  ss << "the string is world, the num is 0, the char is c, the pointer is "
     << ptr;
  std::string expect_str = ss.str();

  // test format with base placeholders
  auto str_after_format1 = FormatString(str_format, "world", 0, 'c', ptr);
  ASSERT_EQ(str_after_format1, expect_str);

  // test format string longer than 100
  std::string long_str(100, 'a');
  std::string long_str_format = long_str + str_format;
  auto str_after_format2 =
      FormatString(long_str_format.c_str(), "world", 0, 'c', ptr);
  ASSERT_EQ(str_after_format2, long_str + expect_str);

  // test empty string
  auto str_after_format3 = FormatString("");
  ASSERT_EQ(str_after_format3, "");

  delete ptr;
}

TEST(AppendStringTest, EmptyTest) {
  auto empty = AppendString();
  ASSERT_EQ(empty, "");

  empty = AppendString("");
  ASSERT_EQ(empty, "");

  empty = AppendString("", "", "");
  ASSERT_EQ(empty, "");
}

TEST(AppendStringTest, StdStringTest) {
  std::string hello{"hello"};
  std::string world{" world"};
  std::string suffix{"!"};

  auto result = AppendString(hello, world, suffix);
  static_assert(std::is_same_v<decltype(result), std::string>);
  ASSERT_EQ(result, "hello world!");
}

TEST(AppendStringTest, StdStringWithCStringTest) {
  std::string hello{"hello"};

  auto result = AppendString(hello, " world", "!");
  static_assert(std::is_same_v<decltype(result), std::string>);
  ASSERT_EQ(result, "hello world!");
}

TEST(AppendStringTest, StdStringWithNonStringTest) {
  std::string hello{"hello world"};

  auto result = AppendString(hello, " nullptr: ", nullptr, " boolean: ", false,
                             " int: ", 0xff);
  static_assert(std::is_same_v<decltype(result), std::string>);
  ASSERT_EQ(result, "hello world nullptr: nullptr boolean: 0 int: 255");
}

namespace {
struct Foo {
  Foo() = default;
  explicit Foo(std::int32_t x) : x_(x){};
  friend std::stringstream& operator<<(std::stringstream& output,
                                       const Foo& foo) {
    output << foo.x_;
    return output;
  }

 private:
  std::int32_t x_;
};
}  // namespace

TEST(AppendStringTest, CustomOperatorTest) {
  auto result = AppendString(Foo{}, Foo{1}, Foo{2});

  ASSERT_EQ(result, "012");
}

TEST(StringUtils, SplitString) {
  {
    std::vector<std::string> parts;
    SplitString(" abc,e  , 11, 3 ", ',', true,
                [&](const char* s, size_t len, int index) {
                  auto full = std::to_string(index) + ':' + std::string(s, len);
                  parts.push_back(std::move(full));
                  return true;
                });
    EXPECT_TRUE(parts.size() == 4);
    EXPECT_EQ(parts[0], "0:abc");
    EXPECT_EQ(parts[1], "1:e");
    EXPECT_EQ(parts[2], "2:11");
    EXPECT_EQ(parts[3], "3:3");
  }
  {
    std::vector<std::string> parts;
    SplitString(" abc,e  , 11, 3 ", ',', false,
                [&](const char* s, size_t len, int index) {
                  auto full = std::to_string(index) + ':' + std::string(s, len);
                  parts.push_back(std::move(full));
                  return true;
                });
    EXPECT_TRUE(parts.size() == 4);
    EXPECT_EQ(parts[0], "0: abc");
    EXPECT_EQ(parts[1], "1:e  ");
    EXPECT_EQ(parts[2], "2: 11");
    EXPECT_EQ(parts[3], "3: 3 ");
  }
  {
    std::vector<std::string> parts;
    SplitString(" abc, ,e  , 11,", ',', true,
                [&](const char* s, size_t len, int index) {
                  auto full = std::to_string(index) + ':' + std::string(s, len);
                  parts.push_back(std::move(full));
                  return true;
                });
    EXPECT_TRUE(parts.size() == 3);
    EXPECT_EQ(parts[0], "0:abc");
    EXPECT_EQ(parts[1], "1:e");
    EXPECT_EQ(parts[2], "2:11");
  }
  {
    std::vector<std::string> parts;
    SplitString(" ,  ", ',', false, [&](const char* s, size_t len, int index) {
      auto full = std::to_string(index) + ':' + std::string(s, len);
      parts.push_back(std::move(full));
      return true;
    });
    EXPECT_TRUE(parts.size() == 2);
    EXPECT_EQ(parts[0], "0: ");
    EXPECT_EQ(parts[1], "1:  ");
  }
  {
    std::vector<std::string> parts;
    SplitString(" ,  ", ',', true, [&](const char* s, size_t len, int index) {
      auto full = std::to_string(index) + ':' + std::string(s, len);
      parts.push_back(std::move(full));
      return true;
    });
    EXPECT_TRUE(parts.empty());
  }
  {
    std::vector<std::string> parts;
    SplitString("    ", ',', true, [&](const char* s, size_t len, int index) {
      auto full = std::to_string(index) + ':' + std::string(s, len);
      parts.push_back(std::move(full));
      return true;
    });
    EXPECT_TRUE(parts.empty());
  }
  {
    std::vector<std::string> parts;
    SplitString("    ", ',', false, [&](const char* s, size_t len, int index) {
      auto full = std::to_string(index) + ':' + std::string(s, len);
      parts.push_back(std::move(full));
      return true;
    });
    EXPECT_TRUE(parts.size() == 1);
    EXPECT_EQ(parts[0], "0:    ");
  }
}

TEST(StringUtils, all) {
  {
    std::vector<std::string> result;
    EXPECT_FALSE(SplitString("", ' ', result));
    EXPECT_TRUE(SplitString("a bc def ghij", ' ', result));
    EXPECT_EQ((int)result.size(), 4);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "bc");
    EXPECT_EQ(result[2], "def");
    EXPECT_EQ(result[3], "ghij");
  }

  EXPECT_TRUE(EndsWith("abcdeft", "deft"));
  EXPECT_FALSE(EndsWith("", "a"));
  EXPECT_FALSE(EndsWith("abc", "d"));

  EXPECT_TRUE(EndsWithIgnoreSourceCase("abCdE", "cde"));
  EXPECT_EQ(StringToLowerASCII(" !@#$%^123aBcDeF"), " !@#$%^123abcdef");

  EXPECT_EQ(TrimString(" aa "), "aa");
  EXPECT_EQ(TrimString(" a a "), "a a");

  EXPECT_TRUE(EqualsIgnoreCase("12aBcDeF45", "12AbCdEf45"));
}

}  // namespace base
}  // namespace lynx
