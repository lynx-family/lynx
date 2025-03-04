// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/background_position_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(BackgroundPositionHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDBackgroundPosition;
  StyleMap output;
  CSSParserConfigs configs;

  // input invalid.
  auto impl = lepus::Value(111);
  bool ret;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("center");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  auto background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  auto pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  auto arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  for (size_t i = 0; i < arr->size(); i += 2) {
    EXPECT_EQ(
        (uint32_t)arr->get(i).Number(),
        static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter));
    EXPECT_EQ((float)arr->get(i + 1).Number(),
              -1.f * static_cast<uint32_t>(
                         starlight::BackgroundPositionType::kCenter));
  }

  output.clear();
  impl = lepus::Value("left");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((uint32_t)arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BackgroundPositionType::kLeft));
  EXPECT_EQ(
      (float)arr->get(1).Number(),
      -1.f * static_cast<uint32_t>(starlight::BackgroundPositionType::kLeft));
  EXPECT_EQ((uint32_t)arr->get(2).Number(),
            static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter));
  EXPECT_EQ(
      (float)arr->get(3).Number(),
      -1.f * static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter));

  output.clear();
  impl = lepus::Value("bottom");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((uint32_t)arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter));
  EXPECT_EQ(
      (float)arr->get(1).Number(),
      -1.f * static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter));
  EXPECT_EQ((uint32_t)arr->get(2).Number(),
            static_cast<uint32_t>(starlight::BackgroundPositionType::kBottom));
  EXPECT_EQ(
      (float)arr->get(3).Number(),
      -1.f * static_cast<uint32_t>(starlight::BackgroundPositionType::kBottom));

  output.clear();
  impl = lepus::Value("bottom, right bottom");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(2));
  {
    arr = pos->get(0).Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(4));
    EXPECT_EQ(
        (uint32_t)arr->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter));
    EXPECT_EQ((float)arr->get(1).Number(),
              -1.f * static_cast<uint32_t>(
                         starlight::BackgroundPositionType::kCenter));
    EXPECT_EQ(
        (uint32_t)arr->get(2).Number(),
        static_cast<uint32_t>(starlight::BackgroundPositionType::kBottom));
    EXPECT_EQ((float)arr->get(3).Number(),
              -1.f * static_cast<uint32_t>(
                         starlight::BackgroundPositionType::kBottom));
  }
  {
    arr = pos->get(1).Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(4));
    EXPECT_EQ((uint32_t)arr->get(0).Number(),
              static_cast<uint32_t>(starlight::BackgroundPositionType::kRight));
    EXPECT_EQ((float)arr->get(1).Number(),
              -1.f * static_cast<uint32_t>(
                         starlight::BackgroundPositionType::kRight));
    EXPECT_EQ(
        (uint32_t)arr->get(2).Number(),
        static_cast<uint32_t>(starlight::BackgroundPositionType::kBottom));
    EXPECT_EQ((float)arr->get(3).Number(),
              -1.f * static_cast<uint32_t>(
                         starlight::BackgroundPositionType::kBottom));
  }

  output.clear();
  impl = lepus::Value("right bottom");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((uint32_t)arr->get(0).Number(),
            static_cast<uint32_t>(starlight::BackgroundPositionType::kRight));
  EXPECT_EQ(
      (float)arr->get(1).Number(),
      -1.f * static_cast<uint32_t>(starlight::BackgroundPositionType::kRight));
  EXPECT_EQ((uint32_t)arr->get(2).Number(),
            static_cast<uint32_t>(starlight::BackgroundPositionType::kBottom));
  EXPECT_EQ(
      (float)arr->get(3).Number(),
      -1.f * static_cast<uint32_t>(starlight::BackgroundPositionType::kBottom));

  output.clear();
  impl = lepus::Value("50px 40px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PX);
  EXPECT_EQ((float)arr->get(1).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::PX);
  EXPECT_EQ((float)arr->get(3).Number(), 40);

  output.clear();
  impl = lepus::Value("50px 40%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PX);
  EXPECT_EQ((float)arr->get(1).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::PERCENT);
  EXPECT_EQ((float)arr->get(3).Number(), 40);
}

TEST(BackgroundPositionHandler, Swap) {
  std::vector<std::pair<std::string, std::string>> cases = {
      {"top left", "left top"},           {"top right", "right top"},
      {"bottom right", "right bottom"},   {"bottom left", "left bottom"},
      {"bottom center", "center bottom"}, {"top center", "center top"},
      {"left center", "center left"},     {"right center", "center right"},
  };
  CSSParserConfigs configs;
  for (const auto& s : cases) {
    CSSStringParser first{s.first.c_str(),
                          static_cast<uint32_t>(s.first.size()), configs};
    CSSStringParser second{s.second.c_str(),
                           static_cast<uint32_t>(s.second.size()), configs};
    EXPECT_FALSE(first.ParseBackgroundPosition().IsEmpty());
    EXPECT_FALSE(second.ParseBackgroundPosition().IsEmpty());
    EXPECT_EQ(first.ParseBackgroundPosition(),
              second.ParseBackgroundPosition());
  }
}

TEST(BackgroundPositionHandler, Calc) {
  auto id = CSSPropertyID::kPropertyIDBackgroundPosition;
  StyleMap output;
  CSSParserConfigs configs;

  // input invalid.
  output.clear();
  auto impl = lepus::Value("calc(100% - 20px)  40px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  auto background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  auto pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  auto arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::CALC);
  EXPECT_EQ(arr->get(1).StringView(), "calc(100% - 20px)");
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::PX);
  EXPECT_EQ(arr->get(3).Number(), 40);

  output.clear();
  impl = lepus::Value("calc(20px + 50%)  calc(30px * 2)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::CALC);
  EXPECT_EQ(arr->get(1).StringView(), "calc(20px + 50%)");
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::CALC);
  EXPECT_EQ(arr->get(3).StringView(), "calc(30px * 2)");

  output.clear();
  impl = lepus::Value("calc(20px + (20px * 2)) calc(50% + (10px * 2 * 50%))");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_position = output[id];
  EXPECT_TRUE(background_position.IsArray());
  pos = background_position.GetValue().Array();
  EXPECT_EQ(pos->size(), static_cast<size_t>(1));
  arr = pos->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::CALC);
  EXPECT_EQ(arr->get(1).StringView(), "calc(20px + (20px * 2))");
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::CALC);
  EXPECT_EQ(arr->get(3).StringView(), "calc(50% + (10px * 2 * 50%))");
}

TEST(BackgroundPositionHandler, Invalid) {
  CSSParserConfigs configs;
  std::vector<std::string> cases = {
      "left hello", "top 10%",  "top top",
      "right left", "10% left", "50px right",
  };
  for (const auto& s : cases) {
    CSSStringParser parser{s.c_str(), static_cast<uint32_t>(s.size()), configs};
    CSSValue pos = parser.ParseBackgroundPosition();
    EXPECT_TRUE(pos.IsEmpty());
    EXPECT_FALSE(!pos.IsEmpty());
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
