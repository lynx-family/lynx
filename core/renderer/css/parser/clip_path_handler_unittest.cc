// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/clip_path_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
TEST(ClipPathHandler, HandleCircle) {
  CSSParserConfigs configs;
  StyleMap out;
  // valid circle
  lepus::Value input = lepus::Value(R"(circle(40px at 30px bottom))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  CSSValue circle = out[kPropertyIDClipPath];
  auto arr = circle.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kCircle));
  EXPECT_EQ(arr->get(1).Number(), 40);
  EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
  EXPECT_EQ(arr->get(3).Number(), 30);
  EXPECT_EQ(arr->get(4).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
  EXPECT_EQ(arr->get(5).Number(), 100);
  EXPECT_EQ(arr->get(6).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));

  // shorthand circle
  input = lepus::Value(R"(circle(30%))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  circle = out[kPropertyIDClipPath];
  arr = circle.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kCircle));
  EXPECT_EQ(arr->get(1).Number(), 30);
  EXPECT_EQ(arr->get(2).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));
  EXPECT_EQ(arr->get(3).Number(), 50);
  EXPECT_EQ(arr->get(4).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));
  EXPECT_EQ(arr->get(5).Number(), 50);
  EXPECT_EQ(arr->get(6).Number(),
            static_cast<uint32_t>(CSSValuePattern(CSSValuePattern::PERCENT)));

  // invalid circle
  input = lepus::Value(R"(circle(ppp))");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
}

TEST(ClipPathHandler, Handle) {
  CSSParserConfigs configs;
  StyleMap out;
  // Input is not string
  lepus::Value input = lepus::Value(30);
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));

  // Input is string
  // Test parse path
  CSSValue value = CSSValue::Empty();
  input = lepus::Value(R"(path("M 0 0 L 100 100 L 30 30 Z"))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  CSSValue path = out[kPropertyIDClipPath];
  auto arr = path.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kPath));
  EXPECT_TRUE(strcmp(arr->get(1).CString(), "M 0 0 L 100 100 L 30 30 Z") == 0);

  // invalid path
  input = lepus::Value(R"(path(100)");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));

  // valid ellipse
  input = lepus::Value(R"(ellipse(20px 50% at bottom right))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  CSSValue ellipse = out[kPropertyIDClipPath];
  arr = ellipse.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kEllipse));
  EXPECT_EQ(arr->get(1).Number(), 20);
  EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
  EXPECT_EQ(arr->get(3).Number(), 50);
  EXPECT_EQ(arr->get(4).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));
  EXPECT_EQ(arr->get(5).Number(), 100);
  EXPECT_EQ(arr->get(6).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));
  EXPECT_EQ(arr->get(7).Number(), 100);
  EXPECT_EQ(arr->get(8).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));

  input = lepus::Value(R"(ellipse(20px 50ppx at left top))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  ellipse = out[kPropertyIDClipPath];
  arr = ellipse.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kEllipse));
  EXPECT_EQ(arr->get(1).Number(), 20);
  EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
  EXPECT_EQ(arr->get(3).Number(), 50);
  EXPECT_EQ(arr->get(4).Number(), static_cast<uint32_t>(CSSValuePattern::PPX));
  EXPECT_EQ(arr->get(5).Number(), 0);
  EXPECT_EQ(arr->get(6).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));
  EXPECT_EQ(arr->get(7).Number(), 0);
  EXPECT_EQ(arr->get(8).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));

  // invalid ellipse
  input = lepus::Value(R"(ellipse(20px at left center))");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));

  // <percentage> position
  input = lepus::Value(R"(ellipse(20px 50ppx at 35% 20%))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  ellipse = out[kPropertyIDClipPath];
  arr = ellipse.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kEllipse));
  EXPECT_EQ(arr->get(1).Number(), 20);
  EXPECT_EQ(arr->get(2).Number(), static_cast<uint32_t>(CSSValuePattern::PX));
  EXPECT_EQ(arr->get(3).Number(), 50);
  EXPECT_EQ(arr->get(4).Number(), static_cast<uint32_t>(CSSValuePattern::PPX));
  EXPECT_EQ(arr->get(5).Number(), 35);
  EXPECT_EQ(arr->get(6).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));
  EXPECT_EQ(arr->get(7).Number(), 20);
  EXPECT_EQ(arr->get(8).Number(),
            static_cast<uint32_t>(CSSValuePattern::PERCENT));
}

TEST(ClipPathHandler, HandleSuperEllipse) {
  CSSParserConfigs configs;
  StyleMap out;
  // valid super-ellipse

  lepus::Value input =
      lepus::Value(R"(super-ellipse(40px 30px 2 2 at 30px bottom))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  CSSValue super_ellipse = out[kPropertyIDClipPath];
  auto arr = super_ellipse.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kSuperEllipse));
#define UNIT_PX static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PX)
#define UNIT_PERCENT static_cast<uint32_t>(lynx::tasm::CSSValuePattern::PERCENT)
#define CHECK_SUPER_ELLIPSE(arr, rx, urx, ry, ury, ex, ey, px, upx, py, upy) \
  EXPECT_EQ(arr->get(1).Number(), rx);                                       \
  EXPECT_EQ(arr->get(2).Number(), urx);                                      \
  EXPECT_EQ(arr->get(3).Number(), ry);                                       \
  EXPECT_EQ(arr->get(4).Number(), ury);                                      \
  EXPECT_EQ(arr->get(5).Number(), ex);                                       \
  EXPECT_EQ(arr->get(6).Number(), ey);                                       \
  EXPECT_EQ(arr->get(7).Number(), px);                                       \
  EXPECT_EQ(arr->get(8).Number(), upx);                                      \
  EXPECT_EQ(arr->get(9).Number(), py);                                       \
  EXPECT_EQ(arr->get(10).Number(), upy);

  CHECK_SUPER_ELLIPSE(arr, 40, UNIT_PX, 30, UNIT_PX, 2, 2, 30, UNIT_PX, 100,
                      UNIT_PERCENT)

  // test shorthand
  // <shape-radius>{2} at <position>
  input = lepus::Value(R"(super-ellipse(40% 30px at top right))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  super_ellipse = out[kPropertyIDClipPath];
  arr = super_ellipse.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kSuperEllipse));
  CHECK_SUPER_ELLIPSE(arr, 40, UNIT_PERCENT, 30, UNIT_PX, 2, 2, 100,
                      UNIT_PERCENT, 0, UNIT_PERCENT)

  // <shape-radius>{2}
  input = lepus::Value(R"(super-ellipse(100px 20%))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  super_ellipse = out[kPropertyIDClipPath];
  arr = super_ellipse.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kSuperEllipse));
  CHECK_SUPER_ELLIPSE(arr, 100, UNIT_PX, 20, UNIT_PERCENT, 2, 2, 50,
                      UNIT_PERCENT, 50, UNIT_PERCENT)

  // <shape-radius>{2} <number>{2}
  input = lepus::Value(R"(super-ellipse(100px 20% 9 13))");
  EXPECT_TRUE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  super_ellipse = out[kPropertyIDClipPath];
  arr = super_ellipse.GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BasicShapeType::kSuperEllipse));
  CHECK_SUPER_ELLIPSE(arr, 100, UNIT_PX, 20, UNIT_PERCENT, 9, 13, 50,
                      UNIT_PERCENT, 50, UNIT_PERCENT)

  // test invalid
  // <shape-radius>{2} failed
  input = lepus::Value(R"(super-ellipse(100px))");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));

  // [<number>{2}] failed
  input = lepus::Value(R"(super-ellipse(100px 100px 2 at bottom right))");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  input = lepus::Value(R"(super-ellipse(100px 100px 2))");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));

  // [at position failed]
  input = lepus::Value(R"(super-ellipse(100px 100px 2 3 at))");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
  input = lepus::Value(R"(super-ellipse(100px 100px 2 3 at 100))");
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDClipPath, input, out, configs));
}

}  // namespace tasm
}  // namespace lynx
