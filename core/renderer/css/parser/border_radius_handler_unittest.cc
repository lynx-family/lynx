// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/border_radius_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
using namespace starlight;
namespace tasm {
namespace test {

std::vector<CSSPropertyID> radius_array = {
    CSSPropertyID::kPropertyIDBorderTopLeftRadius,
    CSSPropertyID::kPropertyIDBorderTopRightRadius,
    CSSPropertyID::kPropertyIDBorderBottomRightRadius,
    CSSPropertyID::kPropertyIDBorderBottomLeftRadius,
};

TEST(BorderRadiusHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDBorderRadius;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_length_unit_check = false;

  // input invalid.
  auto impl = lepus::Value(111);
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("50%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));

  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(4));
    for (int i = 0; i < 4;) {
      EXPECT_EQ((int)arr->get(i).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(i + 1).Number(),
                CSSValuePattern::PERCENT);
      i += 2;
    }
  }

  output.clear();
  impl = lepus::Value("50px 10%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(4));

    // TOP
    if (id == 0) {
      EXPECT_EQ((int)arr->get(0).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(),
                CSSValuePattern::PERCENT);
      EXPECT_EQ((int)arr->get(2).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("50px           10%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    // TOP
    if (id == 0) {
      EXPECT_EQ((int)arr->get(0).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(),
                CSSValuePattern::PERCENT);
      EXPECT_EQ((int)arr->get(2).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("50px/10%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    // TOP
    if (id == 0) {
      EXPECT_EQ((int)arr->get(0).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else {
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value(" 50px   /  10%    ");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    // TOP
    if (id == 0) {
      EXPECT_EQ((int)arr->get(0).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else {
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("50px 10px/ 10%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    // TOP
    if (id == 0) {
      EXPECT_EQ((int)arr->get(0).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("50px 10px/ 10% 20%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    if (id == 0) {
      // TOP
      EXPECT_EQ((int)arr->get(0).Number(), 50);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 20);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(),
                CSSValuePattern::PERCENT);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("12px 0 /12px 0");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    if (id == 0) {
      // TOP
      EXPECT_EQ((int)arr->get(0).Number(), 12);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 12);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::NUMBER);
      EXPECT_EQ((int)arr->get(2).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::NUMBER);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("10px 0 /12px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    if (id == 0) {
      // TOP
      EXPECT_EQ((int)arr->get(0).Number(), 10);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 12);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::NUMBER);
      EXPECT_EQ((int)arr->get(2).Number(), 12);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("0/0");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    if (id == 0) {
      // TOP
      EXPECT_EQ((int)arr->get(0).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::NUMBER);
      EXPECT_EQ((int)arr->get(2).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::NUMBER);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::NUMBER);
      EXPECT_EQ((int)arr->get(2).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::NUMBER);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }

  output.clear();
  impl = lepus::Value("0 0/0 0");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    if (id == 0) {
      // TOP
      EXPECT_EQ((int)arr->get(0).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::NUMBER);
      EXPECT_EQ((int)arr->get(2).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::NUMBER);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::NUMBER);
      EXPECT_EQ((int)arr->get(2).Number(), 0);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::NUMBER);
    } else if (id == 2) {
      // BOTTOM == TOP
      EXPECT_EQ(arr->get(0),
                output[radius_array[0]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[0]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[0]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[0]].GetValue().Array()->get(3));
    } else {
      // RIGHT == LEFT
      EXPECT_EQ(arr->get(0),
                output[radius_array[1]].GetValue().Array()->get(0));
      EXPECT_EQ(arr->get(1),
                output[radius_array[1]].GetValue().Array()->get(1));
      EXPECT_EQ(arr->get(2),
                output[radius_array[1]].GetValue().Array()->get(2));
      EXPECT_EQ(arr->get(3),
                output[radius_array[1]].GetValue().Array()->get(3));
    }
  }
}

TEST(BorderRadiusHandler, Calc) {
  auto id = CSSPropertyID::kPropertyIDBorderRadius;
  StyleMap output;
  CSSParserConfigs configs;
  auto impl = lepus::Value("calc(12px*3)  3px 6px / 5px calc(20px*2)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    if (id == 0) {
      // TOP
      EXPECT_EQ(arr->get(0).StringView(), "calc(12px*3)");
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::CALC);
      EXPECT_EQ((int)arr->get(2).Number(), 5);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 3);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ(arr->get(2).StringView(), "calc(20px*2)");
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::CALC);
    } else if (id == 2) {
      // BOTTOM
      EXPECT_EQ((int)arr->get(0).Number(), 6);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ((int)arr->get(2).Number(), 5);
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);
    } else {
      // LEFT
      EXPECT_EQ((int)arr->get(0).Number(), 3);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ(arr->get(2).StringView(), "calc(20px*2)");
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::CALC);
    }
  }

  output.clear();
  impl = lepus::Value("calc(20px/5) 3px 6px / calc(2px + 3px) calc(2px +2px) ");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  for (size_t id = 0; id < radius_array.size(); id++) {
    EXPECT_FALSE(output.find(radius_array[id]) == output.end());
    EXPECT_TRUE(output[radius_array[id]].IsArray());
    auto arr = output[radius_array[id]].GetValue().Array();
    if (id == 0) {
      // TOP
      EXPECT_EQ(arr->get(0).StringView(), "calc(20px/5)");
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::CALC);
      EXPECT_EQ(arr->get(2).StringView(), "calc(2px + 3px)");
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::CALC);
    } else if (id == 1) {
      // RIGHT
      EXPECT_EQ((int)arr->get(0).Number(), 3);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ(arr->get(2).StringView(), "calc(2px +2px)");
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::CALC);
    } else if (id == 2) {
      // BOTTOM
      EXPECT_EQ((int)arr->get(0).Number(), 6);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ(arr->get(2).StringView(), "calc(2px + 3px)");
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::CALC);
    } else {
      // LEFT
      EXPECT_EQ((int)arr->get(0).Number(), 3);
      EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PX);
      EXPECT_EQ(arr->get(2).StringView(), "calc(2px +2px)");
      EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::CALC);
    }
  }
}

TEST(BorderRadiusHandler, Longhand) {
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto id = kPropertyIDBorderTopLeftRadius;
  // With one value
  auto impl = lepus::Value("50%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  for (int i = 0; i < 4;) {
    EXPECT_EQ((int)arr->get(i).Number(), 50);
    EXPECT_EQ((CSSValuePattern)arr->get(i + 1).Number(),
              CSSValuePattern::PERCENT);
    i += 2;
  }
  // With two values
  output.clear();
  id = kPropertyIDBorderTopLeftRadius;
  impl = lepus::Value("50% 10px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  // x
  EXPECT_EQ((int)arr->get(0).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PERCENT);
  // y
  EXPECT_EQ((int)arr->get(2).Number(), 10);
  EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);

  output.clear();
  id = kPropertyIDBorderTopRightRadius;
  // In fact, this value is invalid, for compatibility
  impl = lepus::Value("50%/10px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((int)arr->get(0).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PERCENT);
  EXPECT_EQ((int)arr->get(2).Number(), 10);
  EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::PX);

  output.clear();
  id = kPropertyIDBorderTopRightRadius;
  impl = lepus::Value("50%/calc(10px+10px)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((int)arr->get(0).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(1).Number(), CSSValuePattern::PERCENT);
  EXPECT_EQ(arr->get(2).StringView(), "calc(10px+10px)");
  EXPECT_EQ((CSSValuePattern)arr->get(3).Number(), CSSValuePattern::CALC);
}

TEST(BorderRadiusHandler, Invalid) {
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto id = kPropertyIDBorderRadius;
  // this will be ignored.
  auto impl = lepus::Value("hello");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));

  output.clear();
  impl = lepus::Value("100test/0");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));

  output.clear();
  impl = lepus::Value("100test 100/0");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
}

TEST(BorderRadiusHandler, LengthUnitCheck) {
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto id = kPropertyIDBorderRadius;
  output.clear();
  auto impl = lepus::Value("0/0");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  impl = lepus::Value("0 /     0");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  impl = lepus::Value("100/0");
  configs.enable_length_unit_check = true;
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  configs.enable_length_unit_check = false;
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
