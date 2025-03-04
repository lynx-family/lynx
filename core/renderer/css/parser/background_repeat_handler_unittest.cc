// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/background_repeat_handler.h"

#include <string>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(BackgroundRepeatHandler, One) {
  auto id = CSSPropertyID::kPropertyIDBackgroundRepeat;
  StyleMap output;
  CSSParserConfigs configs;

  std::vector<
      std::pair<std::string, std::pair<starlight::BackgroundRepeatType,
                                       starlight::BackgroundRepeatType>>>
      cases = {{"repeat",
                {starlight::BackgroundRepeatType::kRepeat,
                 starlight::BackgroundRepeatType::kRepeat}},
               {"no-repeat",
                {starlight::BackgroundRepeatType::kNoRepeat,
                 starlight::BackgroundRepeatType::kNoRepeat}},
               {"repeat-x",
                {starlight::BackgroundRepeatType::kRepeat,
                 starlight::BackgroundRepeatType::kNoRepeat}},
               {"repeat-y",
                {starlight::BackgroundRepeatType::kNoRepeat,
                 starlight::BackgroundRepeatType::kRepeat}},
               {"round",
                {starlight::BackgroundRepeatType::kRound,
                 starlight::BackgroundRepeatType::kRound}},
               {"space",
                {starlight::BackgroundRepeatType::kSpace,
                 starlight::BackgroundRepeatType::kSpace}}};

  for (const auto& item : cases) {
    output.clear();
    auto impl = lepus::Value(item.first);
    EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find(id) != output.end());
    EXPECT_EQ(output.size(), static_cast<size_t>(1));
    auto background_size = output[id];
    EXPECT_TRUE(background_size.IsArray());
    auto size = background_size.GetValue().Array();
    EXPECT_EQ(size->size(), static_cast<size_t>(1));
    auto arr = size->get(0).Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(2));
    EXPECT_EQ(arr->get(0).Number(), static_cast<int>(item.second.first));
    EXPECT_EQ(arr->get(1).Number(), static_cast<int>(item.second.second));
  }
}

TEST(BackgroundRepeatHandler, Two) {
  auto id = CSSPropertyID::kPropertyIDBackgroundRepeat;
  StyleMap output;
  CSSParserConfigs configs;

  std::vector<
      std::pair<std::string, std::pair<starlight::BackgroundRepeatType,
                                       starlight::BackgroundRepeatType>>>
      cases = {{"repeat no-repeat",
                {starlight::BackgroundRepeatType::kRepeat,
                 starlight::BackgroundRepeatType::kNoRepeat}},
               {"no-repeat repeat",
                {starlight::BackgroundRepeatType::kNoRepeat,
                 starlight::BackgroundRepeatType::kRepeat}},
               {"round round",
                {starlight::BackgroundRepeatType::kRound,
                 starlight::BackgroundRepeatType::kRound}},
               {"space space",
                {starlight::BackgroundRepeatType::kSpace,
                 starlight::BackgroundRepeatType::kSpace}}};

  for (const auto& item : cases) {
    output.clear();
    auto impl = lepus::Value(item.first);
    EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find(id) != output.end());
    EXPECT_EQ(output.size(), static_cast<size_t>(1));
    auto background_size = output[id];
    EXPECT_TRUE(background_size.IsArray());
    auto size = background_size.GetValue().Array();
    EXPECT_EQ(size->size(), static_cast<size_t>(1));
    auto arr = size->get(0).Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(2));
    EXPECT_EQ(arr->get(0).Number(), static_cast<int>(item.second.first));
    EXPECT_EQ(arr->get(1).Number(), static_cast<int>(item.second.second));
  }
}

static void CheckLepusArrayNumberValue(fml::RefPtr<lepus::CArray> left,
                                       fml::RefPtr<lepus::CArray> right) {
  EXPECT_EQ(left->size(), right->size());
  for (int i = 0; i < left->size(); i++) {
    if (left->get(i).IsArray()) {
      EXPECT_TRUE(right->get(i).IsArray());
      CheckLepusArrayNumberValue(left->get(i).Array(), right->get(i).Array());
    } else {
      EXPECT_EQ(left->get(i).Number(), right->get(i).Number());
    }
  }
}

TEST(BackgroundRepeatHandler, Valid) {
  const char* values[] = {
      "repeat-x",
      "repeat-y",
      "repeat",
      "space",
      "round",
      "no-repeat",
      "repeat space",
      "round no-repeat",
      "repeat repeat, repeat",
      "repeat-x, repeat-y, repeat",
      nullptr,
  };
  constexpr int test_value_num = sizeof(values) / sizeof(char*) - 1;
#define make_a_repeat(x, y)                                               \
  ((Repeat){static_cast<uint32_t>(starlight::BackgroundRepeatType::k##x), \
            static_cast<uint32_t>(starlight::BackgroundRepeatType::k##y)})

  typedef struct Repeat {
    uint32_t x;
    uint32_t y;
  } Repeat;

  auto build_expected_repeat_array = [](Repeat* expected,
                                        int num) -> fml::RefPtr<lepus::CArray> {
    fml::RefPtr<lepus::CArray> res = lepus::CArray::Create();
    for (int i = 0; i < num; i++) {
      auto array = lepus::CArray::Create();
      array->push_back(lepus::Value((expected + i)->x));
      array->push_back(lepus::Value((expected + i)->y));
      res->push_back(lepus::Value(array));
    }
    return res;
  };
  fml::RefPtr<lepus::CArray> expected[test_value_num];
  // repeat-x
  Repeat repeat = make_a_repeat(Repeat, NoRepeat);
  expected[0] = build_expected_repeat_array(&repeat, 1);
  // repeat-y
  repeat = make_a_repeat(NoRepeat, Repeat);
  // repeat
  expected[1] = build_expected_repeat_array(&repeat, 1);
  repeat = make_a_repeat(Repeat, Repeat);
  expected[2] = build_expected_repeat_array(&repeat, 1);
  // space
  repeat = make_a_repeat(Space, Space);
  expected[3] = build_expected_repeat_array(&repeat, 1);
  // round
  repeat = make_a_repeat(Round, Round);
  expected[4] = build_expected_repeat_array(&repeat, 1);
  //"no-repeat",
  repeat = make_a_repeat(NoRepeat, NoRepeat);
  expected[5] = build_expected_repeat_array(&repeat, 1);
  //      "repeat space",
  repeat = make_a_repeat(Repeat, Space);
  expected[6] = build_expected_repeat_array(&repeat, 1);
  //      "round no-repeat",
  repeat = make_a_repeat(Round, NoRepeat);
  expected[7] = build_expected_repeat_array(&repeat, 1);
  //      "repeat repeat, repeat",
  Repeat repeats[] = {make_a_repeat(Repeat, Repeat),
                      make_a_repeat(Repeat, Repeat)};

  expected[8] = build_expected_repeat_array(repeats, 2);
  //      "repeat-x, repeat-y, repeat",
  Repeat repeats_1[] = {make_a_repeat(Repeat, NoRepeat),
                        make_a_repeat(NoRepeat, Repeat),
                        make_a_repeat(Repeat, Repeat)};
  expected[9] = build_expected_repeat_array(repeats_1, 3);
#undef make_a_repeat

  // test loop
  CSSParserConfigs configs;
  int i = 0;
  for (const char** it = values + i; *it; it = ++i + values) {
    CSSStringParser parser = {*it, static_cast<uint32_t>(strlen(*it)), configs};
    CSSValue repeat_array = parser.ParseBackgroundRepeat();
    EXPECT_TRUE(repeat_array.IsArray());
    CheckLepusArrayNumberValue(repeat_array.GetValue().Array(), expected[i]);
  }
}

TEST(BackgroundRepeatHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDBackgroundRepeat;
  StyleMap output;
  CSSParserConfigs configs;

  auto cases = {"repeat-y repeat-x", "repeat-x no-repeat"};

  for (const auto& item : cases) {
    output.clear();
    auto impl = lepus::Value(item);
    EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  }
}

TEST(BackgroundRepeatHandler, Invalid2) {
  const char* invalid_values[] = {"repeat-y round", nullptr};
  CSSParserConfigs configs;
  for (const char** it = invalid_values; *it; it++) {
    CSSStringParser parser{*it, static_cast<uint32_t>(strlen(*it)), configs};
    CSSValue repeat = parser.ParseBackgroundRepeat();
    EXPECT_TRUE(repeat.IsEmpty());
  }

  {  // Invalid value, but in Lynx it can be parsed with first two values.
    const char* invalid_value = "repeat space round";
    CSSStringParser parser{
        invalid_value, static_cast<uint32_t>(strlen(invalid_value)), configs};
    CSSValue repeat = parser.ParseBackgroundRepeat();
    EXPECT_TRUE(repeat.IsEmpty());
  }

  // invalid value should be [repeat, repeat] in lynx
  {
    const char* invalid_value = "repeat repeat-x";
    CSSStringParser parser{
        invalid_value, static_cast<uint32_t>(strlen(invalid_value)), configs};
    CSSValue repeat = parser.ParseBackgroundRepeat();
    EXPECT_TRUE(repeat.IsEmpty());
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
