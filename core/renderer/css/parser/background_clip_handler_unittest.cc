// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/background_clip_handler.h"

#include "base/include/value/array.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/starlight/style/css_type.h"
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
    CSSValue clip = parser.ParseBackgroundClip();
    EXPECT_TRUE(clip.IsArray());
    EXPECT_NE(clip.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        clip.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kContentBox));
  }
  {
    // padding-box
    auto raw = R"(padding-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue clip = parser.ParseBackgroundClip();
    EXPECT_TRUE(clip.IsArray());
    EXPECT_NE(clip.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(
        clip.GetValue().Array().get()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kPaddingBox));
  }
  {
    // border-box
    auto raw = R"(border-box)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue clip = parser.ParseBackgroundClip();
    EXPECT_TRUE(clip.IsArray());
    EXPECT_NE(clip.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(clip.GetValue().Array().get()->get(0).Number(),
              static_cast<uint32_t>(starlight::BackgroundClipType::kBorderBox));
  }
  {
    // text
    auto raw = R"(text)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue clip = parser.ParseBackgroundClip();
    EXPECT_TRUE(clip.IsArray());
    EXPECT_NE(clip.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(clip.GetValue().Array().get()->get(0).Number(),
              static_cast<uint32_t>(starlight::BackgroundClipType::kText));
  }
  {
    // multiple value
    auto raw = R"(border-box, padding-box, content-box, text)";
    CSSStringParser parser{raw, static_cast<uint32_t>(strlen(raw)), configs};
    CSSValue clip = parser.ParseBackgroundClip();
    EXPECT_TRUE(clip.IsArray());
    EXPECT_NE(clip.GetValue().Array().get()->size(), 0);
    EXPECT_EQ(clip.GetValue().Array().get()->get(0).Number(),
              static_cast<uint32_t>(starlight::BackgroundClipType::kBorderBox));
    EXPECT_EQ(
        clip.GetValue().Array().get()->get(1).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kPaddingBox));
    EXPECT_EQ(
        clip.GetValue().Array().get()->get(2).Number(),
        static_cast<uint32_t>(starlight::BackgroundClipType::kContentBox));
    EXPECT_EQ(clip.GetValue().Array().get()->get(3).Number(),
              static_cast<uint32_t>(starlight::BackgroundClipType::kText));
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
