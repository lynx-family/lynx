// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/four_sides_shorthand_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
using namespace starlight;
namespace tasm {
namespace test {

TEST(CSSProperty, GridTemplateHandler_OneLength) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("2px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(2));
  EXPECT_EQ(length_array->get(0).Number(), 2);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::PX);
}

TEST(CSSProperty, GridTemplateHandler_TwoLength) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("2px auto");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(4));
  EXPECT_EQ(length_array->get(0).Number(), 2);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(2).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::ENUM);
}

TEST(CSSProperty, GridTemplateHandler_ThreeLength) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("2rpx auto auto");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(6));
  EXPECT_EQ(length_array->get(0).Number(), 2);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::RPX);
  EXPECT_EQ(length_array->get(2).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(4).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::ENUM);
}

TEST(CSSProperty, GridTemplateHandler_OneLengthAndRepeatOne) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("2rpx repeat(2, auto)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(6));
  EXPECT_EQ(length_array->get(0).Number(), 2);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::RPX);
  EXPECT_EQ(length_array->get(2).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(4).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::ENUM);
}

TEST(CSSProperty, GridTemplateHandler_OneLengthAndRepeatTwo) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("2rpx repeat(1, auto 100px)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(6));
  EXPECT_EQ(length_array->get(0).Number(), 2);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::RPX);
  EXPECT_EQ(length_array->get(2).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(4).Number(), 100);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::PX);
}

TEST(CSSProperty, GridTemplateHandler_OneLengthAndRepeatFour) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("2rpx repeat(2, auto 100px)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(10));
  EXPECT_EQ(length_array->get(0).Number(), 2);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::RPX);
  EXPECT_EQ(length_array->get(2).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(4).Number(), 100);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(6).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(7).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(8).Number(), 100);
  EXPECT_EQ(length_array->get(9).Number(), (int)tasm::CSSValuePattern::PX);
}

TEST(CSSProperty, GridTemplateHandler_RepeatFourAndTwoLength) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("repeat(2, auto 100px) 2rpx auto");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(12));
  EXPECT_EQ(length_array->get(0).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(2).Number(), 100);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(4).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(6).Number(), 100);
  EXPECT_EQ(length_array->get(7).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(8).Number(), 2);
  EXPECT_EQ(length_array->get(9).Number(), (int)tasm::CSSValuePattern::RPX);
  EXPECT_EQ(length_array->get(0).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::ENUM);
}

TEST(CSSProperty,
     GridTemplateHandler_OneLengthRepeatTwoAndOneLengthAndRepeatTwo) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("auto repeat(2, auto) 120rpx repeat(2, 100vh)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(12));
  EXPECT_EQ(length_array->get(0).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(2).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(4).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(6).Number(), 120);
  EXPECT_EQ(length_array->get(7).Number(), (int)tasm::CSSValuePattern::RPX);
  EXPECT_EQ(length_array->get(8).Number(), 100);
  EXPECT_EQ(length_array->get(9).Number(), (int)tasm::CSSValuePattern::VH);
  EXPECT_EQ(length_array->get(10).Number(), 100);
  EXPECT_EQ(length_array->get(11).Number(), (int)tasm::CSSValuePattern::VH);
}

TEST(CSSProperty, GridTemplateHandler_RepeatOne) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("repeat(1, 100px)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(2));
  EXPECT_EQ(length_array->get(0).Number(), 100);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::PX);
}
TEST(CSSProperty, GridTemplateHandler_RepeatOneAndRepeatOne) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("repeat(1, 100px)repeat(1, 100px)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(4));
  EXPECT_EQ(length_array->get(0).Number(), 100);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(2).Number(), 100);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::PX);
}

TEST(CSSProperty, GridTemplateHandler_RepeatOneAndOneLengthAndRepeatOne) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("repeat(1,100px)100pxrepeat(1,100px)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(6));
  EXPECT_EQ(length_array->get(0).Number(), 100);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(2).Number(), 100);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(4).Number(), 100);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::PX);
}

TEST(CSSProperty, GridTemplateHandler_None) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(0));
}

TEST(CSSProperty, GridTemplateHandler_OneCalc) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("calc(2px + 3rpx)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(2));
  EXPECT_EQ(length_array->get(0).StringView(), "calc(2px + 3rpx)");
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::CALC);
}

TEST(CSSProperty, GridTemplateHandler_TwoCalc) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("calc(2px + 3rpx) calc(100px + (2vh - 100px))");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(4));
  EXPECT_EQ(length_array->get(0).StringView(), "calc(2px + 3rpx)");
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(2).StringView(), "calc(100px + (2vh - 100px))");
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::CALC);
}

TEST(CSSProperty, GridTemplateHandler_CalcAndOtherLength) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("calc(2px + (1px - 3rpx)) 100rpx 20vw");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(6));
  EXPECT_EQ(length_array->get(0).StringView(), "calc(2px + (1px - 3rpx))");
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(2).Number(), 100);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::RPX);
  EXPECT_EQ(length_array->get(4).Number(), 20);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::VW);
}

TEST(CSSProperty, GridTemplateHandler_RepeatCalcAndOtherLength) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value(
      "repeat(2, 100px calc(2px + 3rpx) calc(100px + (2vh - 100px))) 100px "
      "2fr repeat(3, calc(2px + 3vw))");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(22));
  EXPECT_EQ(length_array->get(0).Number(), 100);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(2).StringView(), "calc(2px + 3rpx)");
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(4).StringView(), "calc(100px + (2vh - 100px))");
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(6).Number(), 100);
  EXPECT_EQ(length_array->get(7).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(8).StringView(), "calc(2px + 3rpx)");
  EXPECT_EQ(length_array->get(9).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(10).StringView(), "calc(100px + (2vh - 100px))");
  EXPECT_EQ(length_array->get(11).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(12).Number(), 100);
  EXPECT_EQ(length_array->get(13).Number(), (int)tasm::CSSValuePattern::PX);
  EXPECT_EQ(length_array->get(14).Number(), 2);
  EXPECT_EQ(length_array->get(15).Number(), (int)tasm::CSSValuePattern::FR);
  EXPECT_EQ(length_array->get(16).StringView(), "calc(2px + 3vw)");
  EXPECT_EQ(length_array->get(17).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(18).StringView(), "calc(2px + 3vw)");
  EXPECT_EQ(length_array->get(19).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(20).StringView(), "calc(2px + 3vw)");
  EXPECT_EQ(length_array->get(21).Number(), (int)tasm::CSSValuePattern::CALC);
}

TEST(CSSProperty, GridTemplateHandler_Fr) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value("1fr 0.2fr auto 2fr");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(8));
  EXPECT_EQ(length_array->get(0).Number(), 1);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::FR);
  EXPECT_EQ(length_array->get(2).Number(), 0.2);
  EXPECT_EQ(length_array->get(3).Number(), (int)tasm::CSSValuePattern::FR);
  EXPECT_EQ(length_array->get(4).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(6).Number(), 2);
  EXPECT_EQ(length_array->get(7).Number(), (int)tasm::CSSValuePattern::FR);
}

TEST(CSSProperty, GridTemplateHandler_MinMax1) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value(
      "minmax(max-content, calc(10px + 0.5em)) minmax(auto, 4%) "
      "fit-content(calc(10px + 0.5em))");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(14));
  EXPECT_EQ(length_array->get(0).Number(), (int)tasm::CSSFunctionType::MINMAX);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(2).StringView(), "max-content");
  EXPECT_EQ(length_array->get(3).Number(),
            (int)tasm::CSSValuePattern::INTRINSIC);
  EXPECT_EQ(length_array->get(4).StringView(), "calc(10px + 0.5em)");
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(6).Number(), (int)tasm::CSSFunctionType::MINMAX);
  EXPECT_EQ(length_array->get(7).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(8).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(9).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(10).Number(), 4);
  EXPECT_EQ(length_array->get(11).Number(),
            (int)tasm::CSSValuePattern::PERCENT);
  EXPECT_EQ(length_array->get(12).StringView(),
            "fit-content(calc(10px + 0.5em))");
  EXPECT_EQ(length_array->get(13).Number(),
            (int)tasm::CSSValuePattern::INTRINSIC);
}

TEST(CSSProperty, GridTemplateHandler_MinMax) {
  auto id = CSSPropertyID::kPropertyIDGridTemplateRows;
  StyleMap output;
  CSSParserConfigs configs;
  output.clear();
  auto impl = lepus::Value(
      "repeat(2, minmax(max-content, calc(10px + 0.5em))) 1fr auto repeat(1, "
      "minmax(calc(100px + 10vw), auto)) repeat(1, minmax(fit-content(100px), "
      "2fr))");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto length_array = output[id].GetValue().Array();
  EXPECT_EQ(length_array->size(), static_cast<size_t>(28));
  EXPECT_EQ(length_array->get(0).Number(), (int)tasm::CSSFunctionType::MINMAX);
  EXPECT_EQ(length_array->get(1).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(2).StringView(), "max-content");
  EXPECT_EQ(length_array->get(3).Number(),
            (int)tasm::CSSValuePattern::INTRINSIC);
  EXPECT_EQ(length_array->get(4).StringView(), "calc(10px + 0.5em)");
  EXPECT_EQ(length_array->get(5).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(6).Number(), (int)tasm::CSSFunctionType::MINMAX);
  EXPECT_EQ(length_array->get(7).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(8).StringView(), "max-content");
  EXPECT_EQ(length_array->get(9).Number(),
            (int)tasm::CSSValuePattern::INTRINSIC);
  EXPECT_EQ(length_array->get(10).StringView(), "calc(10px + 0.5em)");
  EXPECT_EQ(length_array->get(11).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(12).Number(), 1);
  EXPECT_EQ(length_array->get(13).Number(), (int)tasm::CSSValuePattern::FR);
  EXPECT_EQ(length_array->get(14).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(15).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(16).Number(), (int)tasm::CSSFunctionType::MINMAX);
  EXPECT_EQ(length_array->get(17).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(18).StringView(), "calc(100px + 10vw)");
  EXPECT_EQ(length_array->get(19).Number(), (int)tasm::CSSValuePattern::CALC);
  EXPECT_EQ(length_array->get(20).Number(),
            (int)starlight::LengthValueType::kAuto);
  EXPECT_EQ(length_array->get(21).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(22).Number(), (int)tasm::CSSFunctionType::MINMAX);
  EXPECT_EQ(length_array->get(23).Number(), (int)tasm::CSSValuePattern::ENUM);
  EXPECT_EQ(length_array->get(24).StringView(), "fit-content(100px)");
  EXPECT_EQ(length_array->get(25).Number(),
            (int)tasm::CSSValuePattern::INTRINSIC);
  EXPECT_EQ(length_array->get(26).Number(), 2);
  EXPECT_EQ(length_array->get(27).Number(), (int)tasm::CSSValuePattern::FR);
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
