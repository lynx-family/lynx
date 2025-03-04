// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/background_box_handler.h"

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(BackgroundClipHandler, Valid) {
  CSSParserConfigs configs;
  {
    // content-box
    auto raw = R"(content-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        box.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kContentBox));
  }
  {
    // padding-box
    auto raw = R"(padding-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        box.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kPaddingBox));
  }
  {
    // border-box
    auto raw = R"(border-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(box.GetValue().Array().get()->get(0).Number(),
              static_cast<uint32_t>(starlight::BackgroundClipType::kBorderBox));
  }
  {
    // multiple value
    auto raw = R"(border-box, padding-box, content-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(box.GetValue().Array().get()->get(0).Number(),
              static_cast<uint32_t>(starlight::BackgroundClipType::kBorderBox));
    EXPECT_EQ(
        box.GetValue().Array().get()->get(1).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kPaddingBox));
    EXPECT_EQ(
        box.GetValue().Array().get()->get(2).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kContentBox));
  }
}

TEST(BackgroundOriginHandler, Valid) {
  CSSParserConfigs configs;
  {
    // content-box
    auto raw = R"(content-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        box.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundOriginType::kContentBox));
  }
  {
    // padding-box
    auto raw = R"(padding-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        box.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundOriginType::kPaddingBox));
  }
  {
    // border-box
    auto raw = R"(border-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        box.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundOriginType::kBorderBox));
  }
  {
    // multiple value
    auto raw = R"(border-box, padding-box, content-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsArray());
    EXPECT_NE(box.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        box.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundOriginType::kBorderBox));
    EXPECT_EQ(
        box.GetValue().Array().get()->get(1).Number(),
        static_cast<uint32_t>(starlight::BackgroundOriginType::kPaddingBox));
    EXPECT_EQ(
        box.GetValue().Array().get()->get(2).Number(),
        static_cast<uint32_t>(starlight::BackgroundOriginType::kContentBox));
  }
}

TEST(BackgroundBoxHandler, Invalid) {
  CSSParserConfigs configs;
  auto test_invalid = [configs](const char* val) {
    CSSStringParser parser{val, static_cast<uint32_t>(strlen(val)), configs};
    CSSValue box = parser.ParseBackgroundBox();
    EXPECT_TRUE(box.IsEmpty());
  };
  const char* values[4] = {"fill-box", "margin-box", "stroke-box", "view-box"};
  for (auto* val : values) {
    test_invalid(val);
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
