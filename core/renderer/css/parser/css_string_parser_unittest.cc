// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#include "core/renderer/css/parser/css_string_parser.h"
#undef private

#include <array>
#include <cstring>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/starlight/style/css_type.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(CSSStringParser, offset_rotate_value) {
  CSSParserConfigs configs;
  {
    std::string raw = "auto";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseOffsetRotate();
    EXPECT_TRUE(result.IsNumber());

    auto auto_value = result.Number();
    EXPECT_FLOAT_EQ(auto_value, -1024.0);

    raw = "45deg";
    parser = CSSStringParser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                             configs};
    result = parser.ParseOffsetRotate();
    EXPECT_TRUE(result.IsNumber());
    auto deg_value = result.Number();
    EXPECT_FLOAT_EQ(deg_value, 45.0);

    raw = "100%";
    parser = CSSStringParser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                             configs};
    result = parser.ParseOffsetRotate();
    EXPECT_TRUE(result.IsEmpty());

    raw = "wrongvalue";
    parser = CSSStringParser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                             configs};
    result = parser.ParseOffsetRotate();
    EXPECT_TRUE(result.IsEmpty());
  }
}

TEST(CSSStringParser, parse_cursor) {
  CSSParserConfigs configs;
  {
    std::string cursor = "help";
    CSSStringParser parser{cursor.c_str(), (uint32_t)cursor.size(), configs};
    CSSValue result = parser.ParseCursor();
    EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
    auto cursor_array = result.GetArray();
    EXPECT_EQ(cursor_array->size(), static_cast<size_t>(2));
    EXPECT_EQ(cursor_array->get(0).UInt32(),
              static_cast<uint32_t>(starlight::CursorType::kKeyword));
    EXPECT_EQ(cursor_array->get(1).StdString(), cursor);
  }
  std::string cursor_url = "hand.cur";
  std::string cursor_key = "pointer";
  {
    std::string cursor = "url(" + cursor_url + "), " + cursor_key;
    CSSStringParser parser{cursor.c_str(), (uint32_t)cursor.size(), configs};
    CSSValue result = parser.ParseCursor();
    EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
    auto cursor_array = result.GetArray();
    EXPECT_EQ(cursor_array->size(), static_cast<size_t>(4));
    EXPECT_EQ(cursor_array->get(0).UInt32(),
              static_cast<uint32_t>(starlight::CursorType::kUrl));
    auto url = cursor_array->get(1).Array();
    EXPECT_EQ(url->get(0).StdString(), cursor_url);
    EXPECT_NEAR(url->get(1).Number(), 0, 0.0001);
    EXPECT_NEAR(url->get(2).Number(), 0, 0.0001);
    EXPECT_EQ(cursor_array->get(2).UInt32(),
              static_cast<uint32_t>(starlight::CursorType::kKeyword));
    EXPECT_EQ(cursor_array->get(3).StdString(), cursor_key);
  }
  {
    std::string cursor = "url(" + cursor_url + ") 10 20, " + cursor_key;
    CSSStringParser parser{cursor.c_str(), (uint32_t)cursor.size(), configs};
    CSSValue result = parser.ParseCursor();
    EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
    auto cursor_array = result.GetArray();
    EXPECT_EQ(cursor_array->size(), static_cast<size_t>(4));
    EXPECT_EQ(cursor_array->get(0).UInt32(),
              static_cast<uint32_t>(starlight::CursorType::kUrl));
    auto url = cursor_array->get(1).Array();
    EXPECT_EQ(url->get(0).StdString(), cursor_url);
    EXPECT_NEAR(url->get(1).Number(), 10, 0.0001);
    EXPECT_NEAR(url->get(2).Number(), 20, 0.0001);
    EXPECT_EQ(cursor_array->get(2).UInt32(),
              static_cast<uint32_t>(starlight::CursorType::kKeyword));
    EXPECT_EQ(cursor_array->get(3).StdString(), cursor_key);
  }
  {
    std::string cursor = "url(" + cursor_url + ") 10, " + cursor_key;
    CSSStringParser parser{cursor.c_str(), (uint32_t)cursor.size(), configs};
    CSSValue result = parser.ParseCursor();
    EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
    auto cursor_array = result.GetArray();
    EXPECT_EQ(cursor_array->size(), static_cast<size_t>(4));
    EXPECT_EQ(cursor_array->get(0).UInt32(),
              static_cast<uint32_t>(starlight::CursorType::kUrl));
    auto url = cursor_array->get(1).Array();
    EXPECT_EQ(url->get(0).StdString(), cursor_url);
    EXPECT_NEAR(url->get(1).Number(), 0, 0.0001);
    EXPECT_NEAR(url->get(2).Number(), 0, 0.0001);
    EXPECT_EQ(cursor_array->get(2).UInt32(),
              static_cast<uint32_t>(starlight::CursorType::kKeyword));
    EXPECT_EQ(cursor_array->get(3).StdString(), cursor_key);
  }
}

TEST(CSSStringParser, test_lerp_color) {
  uint32_t start = CSSColor(150, 150, 150, 1).Cast();
  uint32_t end = CSSColor(0, 0, 0, 0).Cast();
  uint32_t mix = CSSStringParser::LerpColor(start, end, 0.f, 1.5f, 1.f);
  EXPECT_EQ(mix >> 16 & 0xFF, 50);
  EXPECT_EQ(mix >> 8 & 0xFF, 50);
  EXPECT_EQ(mix & 0xFF, 50);
  EXPECT_EQ(mix >> 24 & 0xFF, 85);

  mix = CSSStringParser::LerpColor(start, end, 0.8f, 1.2f, 1.f);
  EXPECT_EQ(mix >> 16 & 0xFF, 75);
  EXPECT_EQ(mix >> 8 & 0xFF, 75);
  EXPECT_EQ(mix & 0xFF, 75);
  EXPECT_EQ(mix >> 24 & 0xFF, 128);
}

TEST(CSSStringParse, font_face_src) {
  std::string url = R"(
    url("/fonts/OpenSans-Regular-webfont.woff2") format("woff2"), url("/fonts/OpenSans-Regular-webfont.woff")
      format("woff"), local("PingFang SC"), local(song);
  )";
  CSSParserConfigs configs;
  CSSStringParser parser{url.c_str(), static_cast<uint32_t>(url.size()),
                         configs};

  auto result = parser.ParseFontSrc();

  EXPECT_TRUE(result.IsArray());

  auto arr = result.GetArray();

  EXPECT_EQ(arr->size(), static_cast<size_t>(8));

  // type 1
  EXPECT_TRUE(arr->get(0).IsUInt32());
  EXPECT_EQ(arr->get(0).UInt32(),
            static_cast<uint32_t>(starlight::FontFaceSrcType::kUrl));
  // url 1
  EXPECT_TRUE(arr->get(1).IsString());
  EXPECT_EQ(arr->get(1).StdString(),
            std::string("/fonts/OpenSans-Regular-webfont.woff2"));

  // type 2
  EXPECT_TRUE(arr->get(2).IsUInt32());
  EXPECT_EQ(arr->get(2).UInt32(),
            static_cast<uint32_t>(starlight::FontFaceSrcType::kUrl));

  // url 2
  EXPECT_TRUE(arr->get(3).IsString());
  EXPECT_EQ(arr->get(3).StdString(),
            std::string("/fonts/OpenSans-Regular-webfont.woff"));

  // type 3
  EXPECT_TRUE(arr->get(4).IsUInt32());
  EXPECT_EQ(arr->get(4).UInt32(),
            static_cast<uint32_t>(starlight::FontFaceSrcType::kLocal));

  // local name 3
  EXPECT_TRUE(arr->get(5).IsString());
  EXPECT_EQ(arr->get(5).StdString(), std::string("PingFang SC"));

  // type 4
  EXPECT_TRUE(arr->get(6).IsUInt32());
  EXPECT_EQ(arr->get(6).UInt32(),
            static_cast<uint32_t>(starlight::FontFaceSrcType::kLocal));

  // local name 4
  EXPECT_TRUE(arr->get(7).IsString());
  EXPECT_EQ(arr->get(7).StdString(), std::string("song"));
}

TEST(CSSStringParse, font_face_src_failed) {
  std::string url1 = R"(
    url("/fonts/OpenSans-Regular-webfont.woff2") url("/fonts/OpenSans-Regular-webfont.woff")
      format("woff"), local("PingFang SC");
  )";

  std::string url2 = R"(
    url("/fonts/OpenSans-Regular-webfont.woff2") local("PingFang SC");
  )";

  std::string url3 = R"(
    local("PingFang SC") url("/fonts/OpenSans-Regular-webfont.woff2") ;
  )";

  std::string url4 = R"(
    local("PingFang SC"), , url("/fonts/OpenSans-Regular-webfont.woff2") ;
  )";

  CSSParserConfigs configs;
  {
    CSSStringParser parser{url1.c_str(), static_cast<uint32_t>(url1.size()),
                           configs};

    auto result = parser.ParseFontSrc();

    EXPECT_TRUE(result.IsArray());
    auto arr = result.GetArray();
    EXPECT_TRUE(arr->size() == 0);
  }

  {
    CSSStringParser parser{url2.c_str(), static_cast<uint32_t>(url2.size()),
                           configs};

    auto result = parser.ParseFontSrc();

    EXPECT_TRUE(result.IsArray());
    auto arr = result.GetArray();
    EXPECT_TRUE(arr->size() == 0);
  }

  {
    CSSStringParser parser{url3.c_str(), static_cast<uint32_t>(url3.size()),
                           configs};

    auto result = parser.ParseFontSrc();

    EXPECT_TRUE(result.IsArray());
    auto arr = result.GetArray();
    EXPECT_TRUE(arr->size() == 0);
  }

  {
    CSSStringParser parser{url4.c_str(), static_cast<uint32_t>(url4.size()),
                           configs};

    auto result = parser.ParseFontSrc();

    EXPECT_TRUE(result.IsArray());
    auto arr = result.GetArray();
    EXPECT_TRUE(arr->size() == 0);
  }
}

TEST(CSSStringParse, font_weight_parser) {
  std::string weight = R"(
    normal;
  )";

  std::string weight2 = R"(
    400 600;
  )";

  std::string weight3 = R"(
    477;
  )";
  CSSParserConfigs configs;
  {
    CSSStringParser parser{weight.c_str(), static_cast<uint32_t>(weight.size()),
                           configs};

    auto result = parser.ParseFontWeight();

    EXPECT_TRUE(result.IsArray());

    auto arr = result.GetArray();

    EXPECT_TRUE(arr->size() == 1);
    EXPECT_TRUE(arr->get(0).IsInt32());
    EXPECT_EQ(arr->get(0).Int32(), static_cast<int32_t>(400));
  }

  {
    CSSStringParser parser{weight2.c_str(),
                           static_cast<uint32_t>(weight2.size()), configs};

    auto result = parser.ParseFontWeight();

    EXPECT_TRUE(result.IsArray());

    auto arr = result.GetArray();

    EXPECT_TRUE(arr->size() == 2);
    EXPECT_TRUE(arr->get(0).IsInt32());
    EXPECT_EQ(arr->get(0).Int32(), static_cast<int32_t>(400));

    EXPECT_TRUE(arr->get(1).IsInt32());
    EXPECT_EQ(arr->get(1).Int32(), static_cast<int32_t>(600));
  }

  {
    CSSStringParser parser{weight3.c_str(),
                           static_cast<uint32_t>(weight3.size()), configs};

    auto result = parser.ParseFontWeight();

    EXPECT_TRUE(result.IsArray());

    auto arr = result.GetArray();

    EXPECT_TRUE(arr->size() == 1);
    EXPECT_TRUE(arr->get(0).IsInt32());
    EXPECT_EQ(arr->get(0).Int32(), static_cast<int32_t>(500));
  }
}

TEST(CSSStringParser, length_valid_and_value) {
  const char* valid_value[] = {"auto",
                               "max-content",
                               "0",
                               "0px",
                               "10%",
                               "0.5em",
                               "calc(10% - 0.5em)",
                               "fit-content(10%)",
                               "fit-content(calc(10% - 0.5em))"};
  int len = sizeof(valid_value) / sizeof(char*);
#define make_a_length(value, pattern) \
  CSSValue(lepus::Value(value), lynx::tasm::CSSValuePattern::pattern)
  CSSValue lengths[] = {
      make_a_length(static_cast<int>(starlight::LengthValueType::kAuto), ENUM),
      make_a_length("max-content", INTRINSIC),
      make_a_length(0, NUMBER),
      make_a_length(0, PX),
      make_a_length(10, PERCENT),
      make_a_length(0.5, EM),
      make_a_length("calc(10% - 0.5em)", CALC),
      make_a_length("fit-content(10%)", INTRINSIC),
      make_a_length("fit-content(calc(10% - 0.5em))", INTRINSIC)};
#undef make_a_length

  CSSParserConfigs configs;
  for (int i = 0; i < len; i++) {
    CSSStringParser parser{
        valid_value[i], static_cast<uint32_t>(strlen(valid_value[i])), configs};
    CSSValue length = parser.ParseLength();
    EXPECT_TRUE(length.GetValue().IsEqual(lengths[i].GetValue()));
    EXPECT_EQ(length.GetPattern(), lengths[i].GetPattern());
  }
}

TEST(CSSStringParse, length_invalid) {
  std::string len = R"(
    100 px
  )";
  CSSParserConfigs configs;
  CSSStringParser parser{len.c_str(), static_cast<uint32_t>(len.size()),
                         configs};

  auto res = parser.ParseLength();
  EXPECT_TRUE(res.IsEmpty());
}

TEST(CSSStringScanner, DecimalPointsInNumbers) {
  const char* valid[] = {"0.1px", ".1px", nullptr};
  CSSParserConfigs configs;
  for (const char** it = valid; *it; it++) {
    CSSStringParser parser{*it, static_cast<uint32_t>(strlen(*it)), configs};
    CSSValue length = parser.ParseLength();
    EXPECT_EQ(length.GetNumber(), 0.1);
  }

  // decimal point after digits is invalid.
  const char* invalid = "1.px";
  {
    CSSStringParser parser{invalid, static_cast<uint32_t>(strlen(invalid)),
                           configs};
    CSSValue length = parser.ParseLength();
    EXPECT_TRUE(length.IsEmpty());
  }
}

TEST(CSSStringParser, url) {
  // url without quote and with keywords, such as 'calc' but without
  // parenthesis. Should be parsed correctly.
  {
    std::string raw =
        R"(url(https://ellipse.circle.com/obj/linear-gradient-babel/calcfakq640khkohk/env?x-/intrinsic1679421600&))";
    std::string golden =
        R"(https://ellipse.circle.com/obj/linear-gradient-babel/calcfakq640khkohk/env?x-/intrinsic1679421600&)";
    CSSParserConfigs configs;
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    std::string parsed_url = parser.ParseUrl();
    EXPECT_EQ(parsed_url, golden);
  }
  {
    // Function keywords with parenthesis but not quoted is an undefined
    // behavior. Only ensure not crash.
    std::string raw =
        R"(url(https://ellipse.circle.com/obj/linear-gradient-babel/calc()fakq640khkohk/env?x-/intrinsic1679421600&))";
    CSSParserConfigs configs;
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    std::string parsed_url = parser.ParseUrl();
  }
}

TEST(CSSStringParser, basic_shape_inset_invalid) {
  CSSParserConfigs configs;
  {
    // should close with right parenthesis
    std::string raw = R"(inset(10px)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // <length-percentage>{1,4}. length should have unit or be a percentage
    // value.
    std::string raw = R"(inset(10 10 10 10))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // nothing should appear after right parenthesis
    std::string raw = R"(inset(10px 10px 10px 10px) asd)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // only accept 'round' and 'super-ellipse'
    std::string raw = R"(inset(10px 10px circle))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // invalid number of inset params
    std::string raw = R"(inset(10px 10px 10px 10px 10px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // border-radius
    std::string raw = R"(inset(10px round 10px // 10px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // invalid number of vertical radius params
    std::string raw = R"(inset(10px round 10px 10px 10px 10px 10px / 10px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // invalid number of vertical radius params
    std::string raw =
        R"(inset(10px round 10px 10px / 10px 10px 10px 10px 10px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // invalid number of exponents
    std::string raw =
        R"(inset(10px super-ellipse 10 10px 10px / 10px 10px 10px 10px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }

  {
    // invalid number of exponents
    std::string raw =
        R"(inset(10px super-ellipse 10px 10px / 10px 10px 10px 10px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_FALSE(result.IsArray());
  }
}

TEST(CSSStringParser, basic_shape_inset_rect) {
  // valid inset without rounded corner
  CSSParserConfigs configs;
  {
    std::string raw = R"(inset(5%))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_TRUE(result.IsArray());

    auto arr = result.Array();
    EXPECT_EQ(arr->size(), 9);

    EXPECT_EQ(arr->get(1).Number(), 5);
    EXPECT_EQ(arr->get(2).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
    EXPECT_EQ(arr->get(3).Number(), 5);
    EXPECT_EQ(arr->get(4).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
    EXPECT_EQ(arr->get(5).Number(), 5);
    EXPECT_EQ(arr->get(6).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
    EXPECT_EQ(arr->get(7).Number(), 5);
    EXPECT_EQ(arr->get(8).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
  }
  {
    std::string raw = R"(inset(10px 20px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_TRUE(result.IsArray());

    auto arr = result.Array();
    EXPECT_EQ(arr->size(), 9);

    EXPECT_EQ(arr->get(1).Number(), 10);
    EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(3).Number(), 20);
    EXPECT_EQ(arr->get(4).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(5).Number(), 10);
    EXPECT_EQ(arr->get(6).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(7).Number(), 20);
    EXPECT_EQ(arr->get(8).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
  }
  {
    std::string raw = R"(inset(10px 20% 30ppx))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_TRUE(result.IsArray());

    auto arr = result.Array();
    EXPECT_EQ(arr->size(), 9);

    EXPECT_EQ(arr->get(1).Number(), 10);
    EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(3).Number(), 20);
    EXPECT_EQ(arr->get(4).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
    EXPECT_EQ(arr->get(5).Number(), 30);
    EXPECT_EQ(arr->get(6).Number(),
              static_cast<uint32_t>(CSSValuePattern::PPX));
    EXPECT_EQ(arr->get(7).Number(), 20);
    EXPECT_EQ(arr->get(8).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
  }
  {
    std::string raw = R"(inset(10px 20% 30ppx 40rpx))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_TRUE(result.IsArray());

    auto arr = result.Array();
    EXPECT_EQ(arr->size(), 9);

    EXPECT_EQ(arr->get(1).Number(), 10);
    EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(3).Number(), 20);
    EXPECT_EQ(arr->get(4).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
    EXPECT_EQ(arr->get(5).Number(), 30);
    EXPECT_EQ(arr->get(6).Number(),
              static_cast<uint32_t>(CSSValuePattern::PPX));
    EXPECT_EQ(arr->get(7).Number(), 40);
    EXPECT_EQ(arr->get(8).Number(),
              static_cast<uint32_t>(CSSValuePattern::RPX));
  }
}

TEST(CSSStringParser, basic_shape_inset_rounded) {
  CSSParserConfigs configs;
  {
    std::string raw = R"(inset(10px round 10px / 20px))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_TRUE(result.IsArray());

    auto arr = result.Array();
    EXPECT_EQ(arr->size(), 25);

    // sides
    EXPECT_EQ(arr->get(1).Number(), 10);
    EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(3).Number(), 10);
    EXPECT_EQ(arr->get(4).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(5).Number(), 10);
    EXPECT_EQ(arr->get(6).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(7).Number(), 10);
    EXPECT_EQ(arr->get(8).Number(), static_cast<uint32_t>(CSSValuePattern::PX));

    // top-left
    EXPECT_EQ(arr->get(9).Number(), 10);
    EXPECT_EQ(arr->get(10).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(11).Number(), 20);
    EXPECT_EQ(arr->get(12).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));

    // top-right
    EXPECT_EQ(arr->get(13).Number(), 10);
    EXPECT_EQ(arr->get(14).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(15).Number(), 20);
    EXPECT_EQ(arr->get(16).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));

    // bottom-right
    EXPECT_EQ(arr->get(17).Number(), 10);
    EXPECT_EQ(arr->get(18).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(19).Number(), 20);
    EXPECT_EQ(arr->get(20).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));

    // bottom-left
    EXPECT_EQ(arr->get(21).Number(), 10);
    EXPECT_EQ(arr->get(22).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(23).Number(), 20);
    EXPECT_EQ(arr->get(24).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
  }
  {
    std::string raw = R"(inset(10px round 10px 20ppx / 20px 30% 40%))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_TRUE(result.IsArray());

    auto arr = result.Array();
    EXPECT_EQ(arr->size(), 25);

    // top-left
    EXPECT_EQ(arr->get(9).Number(), 10);
    EXPECT_EQ(arr->get(10).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(11).Number(), 20);
    EXPECT_EQ(arr->get(12).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));

    // top-right
    EXPECT_EQ(arr->get(13).Number(), 20);
    EXPECT_EQ(arr->get(14).Number(),
              static_cast<uint32_t>(CSSValuePattern::PPX));
    EXPECT_EQ(arr->get(15).Number(), 30);
    EXPECT_EQ(arr->get(16).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));

    // bottom-right
    EXPECT_EQ(arr->get(17).Number(), 10);
    EXPECT_EQ(arr->get(18).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(19).Number(), 40);
    EXPECT_EQ(arr->get(20).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));

    // bottom-left
    EXPECT_EQ(arr->get(21).Number(), 20);
    EXPECT_EQ(arr->get(22).Number(),
              static_cast<uint32_t>(CSSValuePattern::PPX));
    EXPECT_EQ(arr->get(23).Number(), 30);
    EXPECT_EQ(arr->get(24).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
  }
}

TEST(CSSStringParser, basic_shape_inset_ellipse_corner) {
  CSSParserConfigs configs;
  {
    std::string raw =
        R"(inset(10px super-ellipse 5 6 10px 20ppx / 20px 30% 40%))";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto result = parser.ParseShapePath();
    EXPECT_TRUE(result.IsArray());

    auto arr = result.Array();
    EXPECT_EQ(arr->size(), 27);

    EXPECT_EQ(arr->get(9).Number(), 5);
    EXPECT_EQ(arr->get(10).Number(), 6);

    // top-left
    EXPECT_EQ(arr->get(11).Number(), 10);
    EXPECT_EQ(arr->get(12).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(13).Number(), 20);
    EXPECT_EQ(arr->get(14).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));

    // top-right
    EXPECT_EQ(arr->get(15).Number(), 20);
    EXPECT_EQ(arr->get(16).Number(),
              static_cast<uint32_t>(CSSValuePattern::PPX));
    EXPECT_EQ(arr->get(17).Number(), 30);
    EXPECT_EQ(arr->get(18).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));

    // bottom-right
    EXPECT_EQ(arr->get(19).Number(), 10);
    EXPECT_EQ(arr->get(20).Number(),
              static_cast<uint32_t>(CSSValuePattern::PX));
    EXPECT_EQ(arr->get(21).Number(), 40);
    EXPECT_EQ(arr->get(22).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));

    // bottom-left
    EXPECT_EQ(arr->get(23).Number(), 20);
    EXPECT_EQ(arr->get(24).Number(),
              static_cast<uint32_t>(CSSValuePattern::PPX));
    EXPECT_EQ(arr->get(25).Number(), 30);
    EXPECT_EQ(arr->get(26).Number(),
              static_cast<uint32_t>(CSSValuePattern::PERCENT));
  }
}

TEST(CSSStringParser, valid_blur_value) {
  constexpr const char* valid_blur[] = {
      "10px",
      "1.5rpx",
  };
  constexpr int value_len = sizeof(valid_blur) / sizeof(char*);
  const CSSValue valid_values[] = {CSSValue(10, CSSValuePattern::PX),
                                   CSSValue(1.5, CSSValuePattern::RPX)};
  for (int i = 0; i < value_len; i++) {
    CSSStringParser parser{valid_blur[i],
                           static_cast<uint32_t>(strlen(valid_blur[i])),
                           CSSParserConfigs()};
    CSSValue blur = parser.ParseFilterValue(starlight::FilterType::kBlur);
    EXPECT_EQ(valid_values[i].GetNumber(), blur.GetNumber());
    EXPECT_EQ(static_cast<uint32_t>(valid_values[i].GetPattern()),
              static_cast<uint32_t>(blur.GetPattern()));
  }
}

TEST(CSSStringParser, invalid_blur_value) {
  const char* invalid_blur_str[] = {" ",   "px",   "not_blur_value",
                                    "10%", "2px9", nullptr};
  for (const char** it = invalid_blur_str; *it; it++) {
    CSSStringParser parser{*it, static_cast<uint32_t>(strlen(*it)),
                           CSSParserConfigs()};
    EXPECT_TRUE(
        parser.ParseFilterValue(starlight::FilterType::kBlur).IsEmpty());
  }
}

TEST(CSSStringParser, valid_grayscale_value) {
  constexpr const char* valid_grayscale_str[] = {"0" /* 0 doesn't need unit*/,
                                                 "50%", "100%", "0.5", ".5"};
  CSSValue grayscale_values[] = {CSSValue(0, CSSValuePattern::PERCENT),
                                 CSSValue(50, CSSValuePattern::PERCENT),
                                 CSSValue(100, CSSValuePattern::PERCENT),
                                 CSSValue(50, CSSValuePattern::PERCENT),
                                 CSSValue(50, CSSValuePattern::PERCENT)};
  constexpr int num_str = sizeof(valid_grayscale_str) / sizeof(char*);
  for (int i = 0; i < num_str; i++) {
    CSSStringParser parser{
        valid_grayscale_str[i],
        static_cast<uint32_t>(strlen(valid_grayscale_str[i])),
        CSSParserConfigs()};
    CSSValue grayscale =
        parser.ParseFilterValue(starlight::FilterType::kGrayscale);
    EXPECT_EQ(grayscale_values[i].GetNumber(), grayscale.GetNumber());
    EXPECT_EQ(static_cast<uint32_t>(grayscale_values[i].GetPattern()),
              static_cast<uint32_t>(grayscale.GetPattern()));
  }
}

TEST(CSSStringParser, valid_brightness_value) {
  constexpr const char* valid_brightness_str[] = {"0" /* 0 doesn't need unit*/,
                                                  "50%", "100%", "0.5", ".5"};
  CSSValue brightness_values[] = {CSSValue(0.0, CSSValuePattern::NUMBER),
                                  CSSValue(0.5, CSSValuePattern::NUMBER),
                                  CSSValue(1.0, CSSValuePattern::NUMBER),
                                  CSSValue(0.5, CSSValuePattern::NUMBER),
                                  CSSValue(0.5, CSSValuePattern::NUMBER)};
  constexpr int num_str = sizeof(valid_brightness_str) / sizeof(char*);
  for (int i = 0; i < num_str; i++) {
    CSSStringParser parser{
        valid_brightness_str[i],
        static_cast<uint32_t>(strlen(valid_brightness_str[i])),
        CSSParserConfigs()};
    CSSValue brightness =
        parser.ParseFilterValue(starlight::FilterType::kBrightness);
    EXPECT_EQ(brightness_values[i].GetNumber(), brightness.GetNumber());
    EXPECT_EQ(static_cast<uint32_t>(brightness_values[i].GetPattern()),
              static_cast<uint32_t>(brightness.GetPattern()));
  }
}

TEST(CSSStringParser, valid_contrast_value) {
  constexpr const char* valid_contrast_str[] = {"0" /* 0 doesn't need unit*/,
                                                "50%", "100%", "0.5", ".5"};
  CSSValue contrast_values[] = {CSSValue(0.0, CSSValuePattern::NUMBER),
                                CSSValue(0.5, CSSValuePattern::NUMBER),
                                CSSValue(1.0, CSSValuePattern::NUMBER),
                                CSSValue(0.5, CSSValuePattern::NUMBER),
                                CSSValue(0.5, CSSValuePattern::NUMBER)};
  constexpr int num_str = sizeof(valid_contrast_str) / sizeof(char*);
  for (int i = 0; i < num_str; i++) {
    CSSStringParser parser{valid_contrast_str[i],
                           static_cast<uint32_t>(strlen(valid_contrast_str[i])),
                           CSSParserConfigs()};
    CSSValue contrast =
        parser.ParseFilterValue(starlight::FilterType::kContrast);
    EXPECT_EQ(contrast_values[i].GetNumber(), contrast.GetNumber());
    EXPECT_EQ(static_cast<uint32_t>(contrast_values[i].GetPattern()),
              static_cast<uint32_t>(contrast.GetPattern()));
  }
}

TEST(CSSStringParser, valid_saturate_value) {
  constexpr const char* valid_saturate_str[] = {"0" /* 0 doesn't need unit*/,
                                                "50%", "100%", "0.5", ".5"};
  CSSValue saturate_values[] = {
      CSSValue(0.0, CSSValuePattern::NUMBER),
      CSSValue(0.5, CSSValuePattern::NUMBER),
      CSSValue(1.0, CSSValuePattern::NUMBER),
      CSSValue(0.5, CSSValuePattern::NUMBER),
      CSSValue(0.5, CSSValuePattern::NUMBER),
  };
  constexpr int num_str = sizeof(valid_saturate_str) / sizeof(char*);
  for (int i = 0; i < num_str; i++) {
    CSSStringParser parser{valid_saturate_str[i],
                           static_cast<uint32_t>(strlen(valid_saturate_str[i])),
                           CSSParserConfigs()};
    CSSValue saturate =
        parser.ParseFilterValue(starlight::FilterType::kSaturate);
    EXPECT_EQ(saturate_values[i].GetNumber(), saturate.GetNumber());
    EXPECT_EQ(static_cast<uint32_t>(saturate_values[i].GetPattern()),
              static_cast<uint32_t>(saturate.GetPattern()));
  }
}

TEST(CSSStringParser, parse_variable_positions) {
  CSSParserConfigs configs;
  // single var without fallback
  {
    std::string raw = "var(--bg-color)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    CSSValue result = parser.ParseVariable();
    EXPECT_TRUE(result.IsVariable());

    // optionals_ is private; test file exposes private members via
    // #define private public trick at top of this test file.
    auto& refs = result.optionals_.Get<CSSValue::VarReferenceField>();
    ASSERT_EQ(refs.size(), 1u);
    auto& ref = refs[0];

    // start should point to the 'v' of "var(" which is index 0
    EXPECT_EQ(ref.start, static_cast<size_t>(0));
    // end should point to the index after the closing ')'
    EXPECT_EQ(ref.end, static_cast<size_t>(raw.size()));
    EXPECT_EQ(ref.Name(raw), "--bg-color");
    EXPECT_TRUE(ref.fallback.empty());
  }

  // var with fallback
  {
    std::string raw = "prefix var(--a, fallback) suffix";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    CSSValue result = parser.ParseVariable();
    EXPECT_TRUE(result.IsVariable());

    auto& refs = result.optionals_.Get<CSSValue::VarReferenceField>();
    ASSERT_EQ(refs.size(), 1u);
    auto& ref = refs[0];

    // locate the var occurrence in the raw string to compute expected start
    size_t idx = raw.find("var(");
    EXPECT_NE(idx, std::string::npos);
    EXPECT_EQ(ref.start, idx);
    EXPECT_EQ(ref.end, idx + std::string("var(--a, fallback)").size());
    EXPECT_EQ(ref.Name(raw), "--a");
    ASSERT_TRUE(!ref.fallback.empty());
    EXPECT_EQ(ref.fallback.str(), " fallback");
  }
}

TEST(CSSStringParser, parse_variable_multiple_and_malformed) {
  CSSParserConfigs configs;

  // multiple var occurrences
  {
    std::string raw = "foo var(--a) middle var(--b, fallback) end";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    CSSValue result = parser.ParseVariable();
    EXPECT_TRUE(result.IsVariable());
    ASSERT_TRUE(result.optionals_.HasValue<CSSValue::VarReferenceField>());

    auto& refs = result.optionals_.Get<CSSValue::VarReferenceField>();
    ASSERT_EQ(refs.size(), 2u);

    size_t first_idx = raw.find("var(");
    size_t second_idx = raw.find("var(", first_idx + 1);

    EXPECT_EQ(refs[0].start, first_idx);
    EXPECT_EQ(refs[0].end, first_idx + std::string("var(--a)").size());
    EXPECT_EQ(refs[0].Name(result.AsStdString()), "--a");
    EXPECT_TRUE(refs[0].fallback.empty());

    EXPECT_EQ(refs[1].start, second_idx);
    EXPECT_EQ(refs[1].end,
              second_idx + std::string("var(--b, fallback)").size());
    EXPECT_EQ(refs[1].Name(result.AsStdString()), "--b");
    ASSERT_FALSE(refs[1].fallback.empty());
    EXPECT_EQ(refs[1].fallback.str(), " fallback");
  }

  // malformed var (missing closing parenthesis) should not produce references
  {
    std::string raw = "start var(--bad end";  // missing ')'
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    CSSValue result = parser.ParseVariable();
    EXPECT_FALSE(result.IsVariable());
    // No valid references should be attached
    EXPECT_TRUE(!result.optionals_.HasValue<CSSValue::VarReferenceField>() ||
                result.optionals_.Get<CSSValue::VarReferenceField>().empty());
  }
}

TEST(CSSStringParser, parse_variable_nested_fallback) {
  CSSParserConfigs configs;
  std::string raw = "var(--a, var(--b))";
  CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                         configs};
  CSSValue result = parser.ParseVariable();
  EXPECT_TRUE(result.IsVariable());
  ASSERT_TRUE(result.optionals_.HasValue<CSSValue::VarReferenceField>());

  auto& refs = result.optionals_.Get<CSSValue::VarReferenceField>();
  ASSERT_EQ(refs.size(), 1u);
  EXPECT_EQ(refs[0].Name(result.AsStdString()), "--a");
  ASSERT_FALSE(refs[0].fallback.empty());
  // fallback should include the nested var text
  EXPECT_NE(refs[0].fallback.str().find("var(--b)"), std::string::npos);
}

TEST(CSSStringParser, parse_variable_with_leading_whitespaces) {
  CSSParserConfigs configs;

  // Test variable with leading whitespaces in name
  {
    std::string raw = "var(  --bg-color  )";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    CSSValue result = parser.ParseVariable();
    EXPECT_TRUE(result.IsVariable());

    auto& refs = result.optionals_.Get<CSSValue::VarReferenceField>();
    ASSERT_EQ(refs.size(), 1u);
    auto& ref = refs[0];

    // start should point to the 'v' of "var(" which is index 0
    EXPECT_EQ(ref.start, static_cast<size_t>(0));
    // end should point to the index after the closing ')'
    EXPECT_EQ(ref.end, static_cast<size_t>(raw.size()));
    // Name should return the variable name without leading/tailing whitespaces
    EXPECT_EQ(ref.Name(raw), "--bg-color");
    EXPECT_TRUE(ref.fallback.empty());
  }

  // Test variable with leading whitespaces and fallback
  {
    std::string raw = "var(  --spacing  ,  10px  )";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    CSSValue result = parser.ParseVariable();
    EXPECT_TRUE(result.IsVariable());

    auto& refs = result.optionals_.Get<CSSValue::VarReferenceField>();
    ASSERT_EQ(refs.size(), 1u);
    auto& ref = refs[0];

    // Name should return the variable name without leading/tailing whitespaces
    EXPECT_EQ(ref.Name(raw), "--spacing");
    ASSERT_FALSE(ref.fallback.empty());
    // Fallback should preserve its whitespaces
    EXPECT_EQ(ref.fallback.str(), "  10px  ");
  }

  // Test variable with tabs and mixed whitespaces
  {
    std::string raw = "var(\t  --main-color\t  )";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    CSSValue result = parser.ParseVariable();
    EXPECT_TRUE(result.IsVariable());

    auto& refs = result.optionals_.Get<CSSValue::VarReferenceField>();
    ASSERT_EQ(refs.size(), 1u);
    auto& ref = refs[0];

    // Name should return the variable name without leading/tailing whitespaces
    EXPECT_EQ(ref.Name(raw), "--main-color");
    EXPECT_TRUE(ref.fallback.empty());
  }
}

TEST(CSSStringParser, invalid_grayscale) {
  const char* invalid_grayscale_str[] = {
      "ab%", "50,5%" /* should have only one value*/, "50.5 percent", nullptr};
  for (const char** it = invalid_grayscale_str; *it; it++) {
    CSSStringParser parser{*it, static_cast<uint32_t>(strlen(*it)),
                           CSSParserConfigs()};
    CSSValue grayscale =
        parser.ParseFilterValue(starlight::FilterType::kGrayscale);
    EXPECT_TRUE(grayscale.IsEmpty());
  }
}

TEST(CSSStringParser, valid_values) {
  const char* valid_blur_values_str[] = {
      "blur(20px)",
      "grayscale(0.5)",
      "none",
      "BlUr(20px)" /* case-insensitive */,
  };
  constexpr int len = sizeof(valid_blur_values_str) / sizeof(char*);
  const int valid_blur_values[][3] = {
      {static_cast<int>(starlight::FilterType::kBlur), 20,
       static_cast<int>(CSSValuePattern::PX)},
      {static_cast<const int>(starlight::FilterType::kGrayscale), 50,
       static_cast<const int>(CSSValuePattern::PERCENT)},
      {static_cast<const int>(starlight::FilterType::kNone), 0, 0},
      {static_cast<int>(starlight::FilterType::kBlur), 20,
       static_cast<int>(CSSValuePattern::PX)}};
  for (int i = 0; i < len; i++) {
    CSSStringParser parser{
        valid_blur_values_str[i],
        static_cast<uint32_t>(strlen(valid_blur_values_str[i])),
        CSSParserConfigs()};
    CSSValue filter = parser.ParseFilter();
    EXPECT_EQ(filter.GetPattern(), CSSValuePattern::ARRAY);
    EXPECT_EQ(filter.GetArray()->get(0).Number(), valid_blur_values[i][0]);
    EXPECT_EQ(filter.GetArray()->get(1).Number(), valid_blur_values[i][1]);
    EXPECT_EQ(filter.GetArray()->get(2).Number(), valid_blur_values[i][2]);
  }
}

TEST(CSSStringParser, invalid_filter_values) {
  const char* invalid_filter_str[] = {
      "abd(2px)",
      "blur(2px), grayscale(2px)" /* multiple filters are not supported */,
      "12px",
      nullptr,
  };
  for (const char** it = invalid_filter_str; *it; it++) {
    CSSStringParser parser{*it, static_cast<uint32_t>(strlen(*it)),
                           CSSParserConfigs()};
    CSSValue filter = parser.ParseFilter();
    EXPECT_TRUE(filter.IsEmpty());
  }
}

TEST(CSSStringParser, background_image_none) {
  const char* background_image_none = "none";
  CSSStringParser parser{background_image_none,
                         static_cast<uint32_t>(strlen(background_image_none)),
                         CSSParserConfigs()};
  CSSValue css_value_none = parser.ParseBackgroundImage();
  EXPECT_FALSE(css_value_none.IsEmpty());
  EXPECT_TRUE(css_value_none.IsArray());
  EXPECT_EQ(css_value_none.GetArray()->size(), 1);
  EXPECT_EQ(css_value_none.GetArray()->get(0).Number(), 0);
}

TEST(CSSStringParser, aspect_ratio_value) {
  const char* aspect_ratio_none = " ";
  CSSStringParser parser_1{aspect_ratio_none,
                           static_cast<uint32_t>(strlen(aspect_ratio_none)),
                           CSSParserConfigs()};
  CSSValue aspect_ratio_none_css_value = parser_1.ParseAspectRatio();
  EXPECT_TRUE(aspect_ratio_none_css_value.IsEmpty());

  const char* aspect_ratio_single = "6";
  CSSStringParser parser_2{aspect_ratio_single,
                           static_cast<uint32_t>(strlen(aspect_ratio_single)),
                           CSSParserConfigs()};
  CSSValue aspect_ratio_single_css_value = parser_2.ParseAspectRatio();
  EXPECT_EQ(aspect_ratio_single_css_value.GetNumber(), 6.0);
  EXPECT_EQ(aspect_ratio_single_css_value.GetPattern(),
            CSSValuePattern::NUMBER);

  const char* aspect_ratio_divide = "1/2";
  CSSStringParser parser_3{aspect_ratio_divide,
                           static_cast<uint32_t>(strlen(aspect_ratio_divide)),
                           CSSParserConfigs()};
  CSSValue aspect_ratio_divide_css_value = parser_3.ParseAspectRatio();
  EXPECT_EQ(aspect_ratio_divide_css_value.GetNumber(), 0.5);
  EXPECT_EQ(aspect_ratio_divide_css_value.GetPattern(),
            CSSValuePattern::NUMBER);

  const char* aspect_ratio_divide_zero = "2/0";
  CSSStringParser parser_4{
      aspect_ratio_divide_zero,
      static_cast<uint32_t>(strlen(aspect_ratio_divide_zero)),
      CSSParserConfigs()};
  CSSValue aspect_ratio_divide_zero_css_value = parser_4.ParseAspectRatio();
  EXPECT_TRUE(aspect_ratio_divide_zero_css_value.IsEmpty());

  const char* aspect_ratio_chaos = "2 chaos 1";
  CSSStringParser parser_5{aspect_ratio_chaos,
                           static_cast<uint32_t>(strlen(aspect_ratio_chaos)),
                           CSSParserConfigs()};
  CSSValue aspect_ratio_chaos_css_value = parser_5.ParseAspectRatio();
  EXPECT_TRUE(aspect_ratio_chaos_css_value.IsEmpty());

  const char* aspect_ratio_percentage = "50%";
  CSSStringParser parser_6{
      aspect_ratio_percentage,
      static_cast<uint32_t>(strlen(aspect_ratio_percentage)),
      CSSParserConfigs()};
  CSSValue aspect_ratio_percentage_css_value = parser_6.ParseAspectRatio();
  EXPECT_TRUE(aspect_ratio_percentage_css_value.IsEmpty());

  const char* aspect_ratio_equal = "2/2";
  CSSStringParser parser_7{aspect_ratio_equal,
                           static_cast<uint32_t>(strlen(aspect_ratio_equal)),
                           CSSParserConfigs()};
  CSSValue aspect_ratio_equal_css_value = parser_7.ParseAspectRatio();
  EXPECT_EQ(aspect_ratio_equal_css_value.GetNumber(), 1.0);
  EXPECT_EQ(aspect_ratio_equal_css_value.GetPattern(), CSSValuePattern::NUMBER);

  const char* aspect_ratio_negative = "-7";
  CSSStringParser parser_8{aspect_ratio_negative,
                           static_cast<uint32_t>(strlen(aspect_ratio_negative)),
                           CSSParserConfigs()};
  CSSValue aspect_ratio_negative_css_value = parser_8.ParseAspectRatio();
  EXPECT_EQ(aspect_ratio_negative_css_value.GetNumber(), -7.0);
  EXPECT_EQ(aspect_ratio_negative_css_value.GetPattern(),
            CSSValuePattern::NUMBER);
}

TEST(CSSStringParser, gap_value) {
  const char* gap_none = " ";
  CSSStringParser parser_1{gap_none, static_cast<uint32_t>(strlen(gap_none)),
                           CSSParserConfigs()};
  auto gap_none_value = parser_1.ParseGap();
  EXPECT_EQ(gap_none_value.first.GetNumber(), 0);
  EXPECT_EQ(gap_none_value.first.GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(gap_none_value.second.GetNumber(), 0);
  EXPECT_EQ(gap_none_value.second.GetPattern(), CSSValuePattern::PX);

  const char* gap_single = "10px";
  CSSStringParser parser_2{gap_single,
                           static_cast<uint32_t>(strlen(gap_single)),
                           CSSParserConfigs()};
  auto gap_single_value = parser_2.ParseGap();
  EXPECT_EQ(gap_single_value.first.GetNumber(), 10);
  EXPECT_EQ(gap_single_value.first.GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(gap_single_value.second.GetNumber(), 10);
  EXPECT_EQ(gap_single_value.second.GetPattern(), CSSValuePattern::PX);

  const char* gap_double = "10px 20px";
  CSSStringParser parser_3{gap_double,
                           static_cast<uint32_t>(strlen(gap_double)),
                           CSSParserConfigs()};
  auto gap_double_value = parser_3.ParseGap();
  EXPECT_EQ(gap_double_value.first.GetNumber(), 10);
  EXPECT_EQ(gap_double_value.first.GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(gap_double_value.second.GetNumber(), 20);
  EXPECT_EQ(gap_double_value.second.GetPattern(), CSSValuePattern::PX);

  const char* gap_single_prev_wrong = "abc 20px";
  CSSStringParser parser_4{gap_single_prev_wrong,
                           static_cast<uint32_t>(strlen(gap_single_prev_wrong)),
                           CSSParserConfigs()};
  auto gap_single_prev_wrong_value = parser_4.ParseGap();
  EXPECT_EQ(gap_single_prev_wrong_value.first.GetNumber(), 0);
  EXPECT_EQ(gap_single_prev_wrong_value.first.GetPattern(),
            CSSValuePattern::PX);
  EXPECT_EQ(gap_single_prev_wrong_value.second.GetNumber(), 20);
  EXPECT_EQ(gap_single_prev_wrong_value.second.GetPattern(),
            CSSValuePattern::PX);

  const char* gap_single_next_wrong = "30px cde";
  CSSStringParser parser_5{gap_single_next_wrong,
                           static_cast<uint32_t>(strlen(gap_single_next_wrong)),
                           CSSParserConfigs()};
  auto gap_single_next_wrong_value = parser_5.ParseGap();
  EXPECT_EQ(gap_single_next_wrong_value.first.GetNumber(), 30);
  EXPECT_EQ(gap_single_next_wrong_value.first.GetPattern(),
            CSSValuePattern::PX);
  EXPECT_EQ(gap_single_next_wrong_value.second.GetNumber(), 0);
  EXPECT_EQ(gap_single_next_wrong_value.second.GetPattern(),
            CSSValuePattern::PX);

  const char* gap_all_wrong = "fghijk";
  CSSStringParser parser_6{gap_all_wrong,
                           static_cast<uint32_t>(strlen(gap_all_wrong)),
                           CSSParserConfigs()};
  auto gap_all_wrong_value = parser_6.ParseGap();
  EXPECT_EQ(gap_all_wrong_value.first.GetNumber(), 0);
  EXPECT_EQ(gap_all_wrong_value.first.GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(gap_all_wrong_value.second.GetNumber(), 0);
  EXPECT_EQ(gap_all_wrong_value.second.GetPattern(), CSSValuePattern::PX);

  const char* gap_single_percent = "40%";
  CSSStringParser parser_7{gap_single_percent,
                           static_cast<uint32_t>(strlen(gap_single_percent)),
                           CSSParserConfigs()};
  auto gap_single_percent_value = parser_7.ParseGap();
  EXPECT_EQ(gap_single_percent_value.first.GetNumber(), 40);
  EXPECT_EQ(gap_single_percent_value.first.GetPattern(),
            CSSValuePattern::PERCENT);
  EXPECT_EQ(gap_single_percent_value.second.GetNumber(), 40);
  EXPECT_EQ(gap_single_percent_value.second.GetPattern(),
            CSSValuePattern::PERCENT);
}

TEST(CSSStringParser, RGBColor) {
  CSSParserConfigs configs;

  // Test legacy syntax: rgb(R, G, B)
  {
    std::string rgb = "rgb(255, 128, 0)";
    CSSStringParser parser{rgb.c_str(), static_cast<uint32_t>(rgb.size()),
                           configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);
    EXPECT_FALSE(result.IsEmpty());
    EXPECT_EQ(result.GetPattern(), CSSValuePattern::NUMBER);

    uint32_t color = result.GetValue().UInt32();
    EXPECT_EQ((color >> 16) & 0xFF, 255);  // R
    EXPECT_EQ((color >> 8) & 0xFF, 128);   // G
    EXPECT_EQ(color & 0xFF, 0);            // B
    EXPECT_EQ((color >> 24) & 0xFF, 255);  // A (default 1.0)
  }

  // Test legacy syntax with alpha: rgb(R, G, B, A)
  {
    std::string rgba = "rgb(255, 128, 0, 0.5)";
    CSSStringParser parser{rgba.c_str(), static_cast<uint32_t>(rgba.size()),
                           configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);

    uint32_t color = result.GetValue().UInt32();
    EXPECT_EQ((color >> 24) & 0xFF, 127);  // A (0.5 * 255)
  }

  // Test modern syntax: rgb(R G B)
  {
    std::string rgb = "rgb(255 128 0)";
    CSSStringParser parser{rgb.c_str(), static_cast<uint32_t>(rgb.size()),
                           configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);

    uint32_t color = result.GetValue().UInt32();
    EXPECT_EQ((color >> 16) & 0xFF, 255);
    EXPECT_EQ((color >> 8) & 0xFF, 128);
    EXPECT_EQ(color & 0xFF, 0);
  }

  // Test modern syntax with alpha: rgb(R G B / A)
  {
    std::string rgba = "rgb(255 128 0 / 0.5)";
    CSSStringParser parser{rgba.c_str(), static_cast<uint32_t>(rgba.size()),
                           configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);

    uint32_t color = result.GetValue().UInt32();
    EXPECT_EQ((color >> 24) & 0xFF, 127);
  }

  // Test percentage values
  {
    std::string rgb = "rgb(100% 50% 0%)";
    CSSStringParser parser{rgb.c_str(), static_cast<uint32_t>(rgb.size()),
                           configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);

    uint32_t color = result.GetValue().UInt32();
    EXPECT_EQ((color >> 16) & 0xFF, 255);  // 100% of 255
    EXPECT_EQ((color >> 8) & 0xFF, 128);   // 50% of 255
    EXPECT_EQ(color & 0xFF, 0);            // 0% of 255
  }

  // Test 'none' values (modern syntax only)
  {
    std::string rgb = "rgb(none 128 none / 0.5)";
    CSSStringParser parser{rgb.c_str(), static_cast<uint32_t>(rgb.size()),
                           configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);

    uint32_t color = result.GetValue().UInt32();
    EXPECT_EQ((color >> 16) & 0xFF, 0);  // none -> 0
    EXPECT_EQ((color >> 8) & 0xFF, 128);
    EXPECT_EQ(color & 0xFF, 0);            // none -> 0
    EXPECT_EQ((color >> 24) & 0xFF, 127);  // alpha 0.5
  }

  // Test invalid syntax
  {
    // Missing closing parenthesis
    std::string invalid = "rgb(255, 128, 0";
    CSSStringParser parser{invalid.c_str(),
                           static_cast<uint32_t>(invalid.size()), configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);
    EXPECT_TRUE(result.IsEmpty());
  }

  // Test invalid legacy syntax with 'none'
  {
    // 'none' not allowed in legacy syntax
    std::string invalid = "rgb(none, 128, 0)";
    CSSStringParser parser{invalid.c_str(),
                           static_cast<uint32_t>(invalid.size()), configs};
    CSSValue result;
    parser.ParseCSSColorTo(result);
    EXPECT_TRUE(result.IsEmpty());
  }
}
TEST(CSSStringParser, open_type_tag) {
  EXPECT_TRUE(CSSStringParser::IsValidOpenTypeTag("aBcD"));
  EXPECT_TRUE(CSSStringParser::IsValidOpenTypeTag("1234"));
  EXPECT_TRUE(CSSStringParser::IsValidOpenTypeTag("ab12"));
  EXPECT_FALSE(CSSStringParser::IsValidOpenTypeTag(""));
  EXPECT_FALSE(CSSStringParser::IsValidOpenTypeTag("abc"));
  EXPECT_FALSE(CSSStringParser::IsValidOpenTypeTag("abcde"));
  EXPECT_FALSE(CSSStringParser::IsValidOpenTypeTag("ab,.c"));
}

TEST(CSSStringParser, font_variation_settings) {
  CSSParserConfigs configs;

  // Normal cases
  {
    std::string raw = R"('wght' 800)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_TRUE(parser.ParseFontVariationSettings(arr));
    EXPECT_EQ(arr->size(), 2);
    EXPECT_EQ(arr->get(0).ToString(), "wght");
    EXPECT_FLOAT_EQ(arr->get(1).Number(), 800.0);
  }
  {
    std::string raw = R"("wdth" 125.0, 'wght' 750)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_TRUE(parser.ParseFontVariationSettings(arr));
    EXPECT_EQ(arr->size(), 4);
    EXPECT_EQ(arr->get(0).ToString(), "wdth");
    EXPECT_FLOAT_EQ(arr->get(1).Number(), 125.0);
    EXPECT_EQ(arr->get(2).ToString(), "wght");
    EXPECT_FLOAT_EQ(arr->get(3).Number(), 750.0);
  }
  {
    std::string raw = R"(normal)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_TRUE(parser.ParseFontVariationSettings(arr));
    EXPECT_EQ(arr->size(), 0);
  }
  // Error cases
  {
    std::string raw = R"('wght')";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_FALSE(parser.ParseFontVariationSettings(arr));
  }
  {
    std::string raw = R"('wght', 100)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_FALSE(parser.ParseFontVariationSettings(arr));
  }
  {
    std::string raw = R"('badtagname' 100)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_FALSE(parser.ParseFontVariationSettings(arr));
  }
}

TEST(CSSStringParser, font_feature_settings) {
  CSSParserConfigs configs;

  // Normal cases
  {
    std::string raw = R"('dlig')";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_TRUE(parser.ParseFontFeatureSettings(arr));
    EXPECT_EQ(arr->size(), 2);
    EXPECT_EQ(arr->get(0).ToString(), "dlig");
    EXPECT_EQ(arr->get(1).Int32(), 1);
  }
  {
    std::string raw = R"(dlig on, smcp off, c2sc 2,aalt 1)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_TRUE(parser.ParseFontFeatureSettings(arr));
    EXPECT_EQ(arr->size(), 8);
    EXPECT_EQ(arr->get(0).ToString(), "dlig");
    EXPECT_EQ(arr->get(1).Int32(), 1);
    EXPECT_EQ(arr->get(2).ToString(), "smcp");
    EXPECT_EQ(arr->get(3).Int32(), 0);
    EXPECT_EQ(arr->get(4).ToString(), "c2sc");
    EXPECT_EQ(arr->get(5).Int32(), 2);
    EXPECT_EQ(arr->get(6).ToString(), "aalt");
    EXPECT_EQ(arr->get(7).Int32(), 1);
  }
  {
    std::string raw = R"('dlig' on, "smcp" off, 'c2sc' 2)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_TRUE(parser.ParseFontFeatureSettings(arr));
    EXPECT_EQ(arr->size(), 6);
    EXPECT_EQ(arr->get(0).ToString(), "dlig");
    EXPECT_EQ(arr->get(1).Int32(), 1);
    EXPECT_EQ(arr->get(2).ToString(), "smcp");
    EXPECT_EQ(arr->get(3).Int32(), 0);
    EXPECT_EQ(arr->get(4).ToString(), "c2sc");
    EXPECT_EQ(arr->get(5).Int32(), 2);
  }
  {
    std::string raw = R"(normal)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_TRUE(parser.ParseFontFeatureSettings(arr));
    EXPECT_EQ(arr->size(), 0);
  }
  // Error cases
  {
    std::string raw = R"('dlig' , on off)";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_FALSE(parser.ParseFontFeatureSettings(arr));
  }
  {
    std::string raw = R"('badtagname')";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_FALSE(parser.ParseFontFeatureSettings(arr));
  }
  {
    std::string raw = R"('dlig',, 'smcp')";
    CSSStringParser parser{raw.c_str(), static_cast<uint32_t>(raw.size()),
                           configs};
    auto arr = lepus::CArray::Create();
    EXPECT_FALSE(parser.ParseFontFeatureSettings(arr));
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
