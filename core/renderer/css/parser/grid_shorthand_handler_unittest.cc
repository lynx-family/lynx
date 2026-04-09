// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/grid_shorthand_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(GridShorthandHandler, DisabledGridColumn) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = false;
  auto impl = lepus::Value("1 / 4");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(GridShorthandHandler, DisabledGridRow) {
  auto id = CSSPropertyID::kPropertyIDGridRow;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = false;
  auto impl = lepus::Value("1 / span 2");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(GridShorthandHandler, GridColumnSingleValueAuto) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("auto");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnStart].AsNumber(), 0);
  EXPECT_TRUE(output[kPropertyIDGridColumnEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnEnd].AsNumber(), 0);
}

TEST(GridShorthandHandler, GridColumnSingleValueNumber) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("3");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnStart].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDGridColumnEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnEnd].AsNumber(), 0);
}

TEST(GridShorthandHandler, GridColumnSingleValueSpan) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("span 2");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnSpan].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnSpan].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDGridColumnEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnEnd].AsNumber(), 0);
}

TEST(GridShorthandHandler, GridColumnTwoValuesNumberSlashNumber) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("1 / 4");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnStart].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDGridColumnEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnEnd].AsNumber(), 4);
}

TEST(GridShorthandHandler, GridColumnTwoValuesSpanSlashNumber) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("span 2 / 5");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnSpan].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnSpan].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDGridColumnEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnEnd].AsNumber(), 5);
}

TEST(GridShorthandHandler, GridColumnTwoValuesNumberSlashSpan) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("2 / span 3");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnStart].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDGridColumnSpan].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnSpan].AsNumber(), 3);
}

TEST(GridShorthandHandler, GridColumnWithWhitespace) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("  1  /  4  ");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnStart].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDGridColumnEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnEnd].AsNumber(), 4);
}

TEST(GridShorthandHandler, GridRowSingleValue) {
  auto id = CSSPropertyID::kPropertyIDGridRow;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("2");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridRowStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowStart].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDGridRowEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowEnd].AsNumber(), 0);
}

TEST(GridShorthandHandler, GridRowTwoValues) {
  auto id = CSSPropertyID::kPropertyIDGridRow;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("1 / span 2");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridRowStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowStart].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDGridRowSpan].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowSpan].AsNumber(), 2);
}

TEST(GridShorthandHandler, GridRowAuto) {
  auto id = CSSPropertyID::kPropertyIDGridRow;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("auto");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridRowStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowStart].AsNumber(), 0);
  EXPECT_TRUE(output[kPropertyIDGridRowEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowEnd].AsNumber(), 0);
}

TEST(GridShorthandHandler, InvalidMultipleSlashes) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("1 / 2 / 3");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(GridShorthandHandler, InvalidEmptyPartBeforeSlash) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value(" / 2");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(GridShorthandHandler, InvalidEmptyPartAfterSlash) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("1 / ");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(GridShorthandHandler, InvalidEmptyString) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(GridShorthandHandler, InvalidNonString) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value(123);
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(GridShorthandHandler, InvalidNegativeSpanOnly) {
  auto id = CSSPropertyID::kPropertyIDGridRow;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_grid_placement_shorthands = true;
  auto impl = lepus::Value("span -2");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  // GridPositionHandler accepts span with any number via atoi.
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridRowSpan].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowSpan].AsNumber(), -2);
  EXPECT_TRUE(output[kPropertyIDGridRowEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridRowEnd].AsNumber(), 0);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
