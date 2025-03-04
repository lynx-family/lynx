// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/transform_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(TransformHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDTransform;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  impl = lepus::Value("translate(1px,");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  impl = lepus::Value("translate(1px, 2px) scale(0.1) rotate(10deg)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(3));
  auto item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(5));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslate);
  EXPECT_EQ(item->get(1).Number(), 1);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
  EXPECT_EQ(item->get(3).Number(), 2);
  EXPECT_EQ(item->get(4).Number(), (int)CSSValuePattern::PX);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(item->get(1).Number(), 0.1);
  item = arr->get(2).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotate);
  EXPECT_EQ(item->get(1).Number(), 10);

  impl = lepus::Value("translate(1px, 2px) scale(1.1,1.5) rotateX(15deg)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(3));
  item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(5));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslate);
  EXPECT_EQ(item->get(1).Number(), 1);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
  EXPECT_EQ(item->get(3).Number(), 2);
  EXPECT_EQ(item->get(4).Number(), (int)CSSValuePattern::PX);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(3));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kScale);
  EXPECT_FLOAT_EQ(item->get(1).Number(), 1.1);
  EXPECT_FLOAT_EQ(item->get(2).Number(), 1.5);
  item = arr->get(2).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateX);
  EXPECT_EQ(item->get(1).Number(), 15);

  impl = lepus::Value("translate3d(1px, 2px, 3px) rotateY(30deg)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(2));
  item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(7));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslate3d);
  EXPECT_EQ(item->get(1).Number(), 1);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
  EXPECT_EQ(item->get(3).Number(), 2);
  EXPECT_EQ(item->get(4).Number(), (int)CSSValuePattern::PX);
  EXPECT_EQ(item->get(5).Number(), 3);
  EXPECT_EQ(item->get(4).Number(), (int)CSSValuePattern::PX);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateY);
  EXPECT_FLOAT_EQ(item->get(1).Number(), 30);

  impl = lepus::Value("rotateX(-30deg) rotateY(-10deg)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(2));
  item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateX);
  EXPECT_EQ(item->get(1).Number(), -30);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateY);
  EXPECT_FLOAT_EQ(item->get(1).Number(), -10);

  impl = lepus::Value("rotateX(-30deg) rotateY(-10deg)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(2));
  item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateX);
  EXPECT_EQ(item->get(1).Number(), -30);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateY);
  EXPECT_FLOAT_EQ(item->get(1).Number(), -10);

  impl = lepus::Value("rotateX(-30deg) rotateY(-10deg)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(2));
  item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateX);
  EXPECT_EQ(item->get(1).Number(), -30);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateY);
  EXPECT_FLOAT_EQ(item->get(1).Number(), -10);

  impl = lepus::Value("scale(-10, 20) translate3d(2px, -4px, 5px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(2));
  item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(3));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kScale);
  EXPECT_EQ(item->get(1).Number(), -10);
  EXPECT_EQ(item->get(2).Number(), 20);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(7));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslate3d);
  EXPECT_FLOAT_EQ(item->get(1).Number(), 2);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
  EXPECT_FLOAT_EQ(item->get(3).Number(), -4);
  EXPECT_EQ(item->get(4).Number(), (int)CSSValuePattern::PX);
  EXPECT_FLOAT_EQ(item->get(5).Number(), 5);
  EXPECT_EQ(item->get(6).Number(), (int)CSSValuePattern::PX);

  impl = lepus::Value(
      "translateX(2px) translateY(3px) translateZ(-4px) rotateX(10deg) "
      "rotateY(20deg) rotateZ(-10deg)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(6));
  item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(3));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslateX);
  EXPECT_EQ(item->get(1).Number(), 2);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
  item = arr->get(1).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(3));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslateY);
  EXPECT_FLOAT_EQ(item->get(1).Number(), 3);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
  item = arr->get(2).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(3));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslateZ);
  EXPECT_FLOAT_EQ(item->get(1).Number(), -4);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
  item = arr->get(3).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateX);
  EXPECT_EQ(item->get(1).Number(), 10);
  item = arr->get(4).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateY);
  EXPECT_EQ(item->get(1).Number(), 20);
  item = arr->get(5).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(2));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kRotateZ);
  EXPECT_EQ(item->get(1).Number(), -10);
}

TEST(TransformHandler, One) {
  auto id = CSSPropertyID::kPropertyIDTransform;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value("translate(1px)");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), 1);
  auto item = arr->get(0).Array();
  EXPECT_EQ(item->size(), static_cast<size_t>(3));
  EXPECT_EQ(item->get(0).Number(), (int)starlight::TransformType::kTranslate);
  EXPECT_EQ(item->get(1).Number(), 1);
  EXPECT_EQ(item->get(2).Number(), (int)CSSValuePattern::PX);
}

TEST(TransformHandler, None) {
  auto id = CSSPropertyID::kPropertyIDTransform;
  StyleMap output;
  CSSParserConfigs configs;
  auto impl = lepus::Value("none");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), 0);
}

TEST(TransformHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDTransform;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_new_transform_handler = true;

  bool ret;
  lepus::Value impl;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("rotate(20)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("skew(20deg, 20)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("scale(20px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("scale(2, 20px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("scale(2, 20px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("translate(1px, 10");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  impl = lepus::Value("translate(1px, 10px, 10px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  // Need three values
  impl = lepus::Value("translate3d(2px, -4px)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
}

TEST(TransformHandler, Compatibility) {
  auto id = CSSPropertyID::kPropertyIDTransform;
  CSSParserConfigs configs;
  std::vector<std::pair<std::string, std::string>> cases = {
      {"rotate(20)", "rotate(20deg)"},
      {"translate(1px, 10px, 10px)", "translate(1px, 10px)"},
  };

  for (const auto& c : cases) {
    StyleMap output;
    auto ret = UnitHandler::Process(id, lepus::Value(c.first), output, configs);
    EXPECT_TRUE(ret);
    StyleMap right;
    UnitHandler::Process(id, lepus::Value(c.second), right, configs);
    EXPECT_EQ(output[id], right[id]);
  }
}
}  // namespace test

}  // namespace tasm
}  // namespace lynx
