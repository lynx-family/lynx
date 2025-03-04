// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/background_size_handler.h"

#include <vector>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(BackgroundSizeHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDBackgroundSize;
  StyleMap output;
  CSSParserConfigs configs;

  // input invalid.
  auto impl = lepus::Value(111);
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  std::vector<std::string> cases = {"auto", "cover", "contain"};

  for (int i = 0; i < cases.size(); ++i) {
    output.clear();
    impl = lepus::Value(cases[i]);
    EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find(id) != output.end());
    EXPECT_EQ(output.size(), static_cast<size_t>(1));
    auto background_size = output[id];
    EXPECT_TRUE(background_size.IsArray());
    auto size = background_size.GetValue().Array();
    EXPECT_EQ(size->size(), static_cast<size_t>(1));
    auto arr = size->get(0).Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(4));
    for (size_t j = 0; j < arr->size(); j += 2) {
      EXPECT_EQ((uint32_t)arr->get(j).Number(),
                static_cast<uint32_t>(CSSValuePattern::NUMBER));
      EXPECT_EQ(
          (float)arr->get(j + 1).Number(),
          -1.f * (static_cast<int>(starlight::BackgroundSizeType::kAuto) + i));
    }
  }

  output.clear();
  impl = lepus::Value("50px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  auto background_size = output[id];
  EXPECT_TRUE(background_size.IsArray());
  auto size = background_size.GetValue().Array();
  EXPECT_EQ(size->size(), static_cast<size_t>(1));
  auto arr = size->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PX);
  EXPECT_EQ((float)arr->get(1).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::NUMBER);
  EXPECT_EQ((float)arr->get(3).Number(),
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto));

  output.clear();
  impl = lepus::Value("50px auto");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_size = output[id];
  EXPECT_TRUE(background_size.IsArray());
  size = background_size.GetValue().Array();
  EXPECT_EQ(size->size(), static_cast<size_t>(1));
  arr = size->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PX);
  EXPECT_EQ((float)arr->get(1).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::NUMBER);
  EXPECT_EQ((float)arr->get(3).Number(),
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto));

  output.clear();
  impl = lepus::Value("50px 40px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_size = output[id];
  EXPECT_TRUE(background_size.IsArray());
  size = background_size.GetValue().Array();
  EXPECT_EQ(size->size(), static_cast<size_t>(1));
  arr = size->get(0).Array();
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
  background_size = output[id];
  EXPECT_TRUE(background_size.IsArray());
  size = background_size.GetValue().Array();
  EXPECT_EQ(size->size(), static_cast<size_t>(1));
  arr = size->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PX);
  EXPECT_EQ((float)arr->get(1).Number(), 50);
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::PERCENT);
  EXPECT_EQ((float)arr->get(3).Number(), 40);

  output.clear();
  impl = lepus::Value("50px, 30px 40%");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  background_size = output[id];
  EXPECT_TRUE(background_size.IsArray());
  size = background_size.GetValue().Array();
  EXPECT_EQ(size->size(), static_cast<size_t>(2));
  {
    arr = size->get(0).Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(4));
    EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PX);
    EXPECT_EQ((float)arr->get(1).Number(), 50);
    EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::NUMBER);
    EXPECT_EQ((float)arr->get(3).Number(),
              -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto));
  }
  {
    arr = size->get(1).Array();
    EXPECT_EQ(arr->size(), static_cast<size_t>(4));
    EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PX);
    EXPECT_EQ((float)arr->get(1).Number(), 30);
    EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::PERCENT);
    EXPECT_EQ((float)arr->get(3).Number(), 40);
  }
}

TEST(BackgroundSizeHandler, Legacy) {
  auto id = CSSPropertyID::kPropertyIDBackgroundSize;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_legacy_parser = true;

  output.clear();
  auto impl = lepus::Value("auto");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  auto background_size = output[id];
  EXPECT_TRUE(background_size.IsArray());
  auto size = background_size.GetValue().Array();
  EXPECT_EQ(size->size(), static_cast<size_t>(1));
  auto arr = size->get(0).Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(4));
  EXPECT_EQ((CSSValuePattern)arr->get(0).Number(), CSSValuePattern::PERCENT);
  EXPECT_EQ((float)arr->get(1).Number(), 100);
  EXPECT_EQ((CSSValuePattern)arr->get(2).Number(), CSSValuePattern::PERCENT);
  EXPECT_EQ((float)arr->get(3).Number(), 100);
}

TEST(BackgroundSizeHandler, Invalid) {
  {
    CSSParserConfigs configs;
    // This is an invalid value for background-size_array according to CSS
    // syntax. But in Lynx the value can be parsed, and the first pair of values
    // take effect.
    const char* val = "1px 2px 3px";
    CSSStringParser parser{val, static_cast<uint32_t>(strlen(val)), configs};
    parser.SetIsLegacyParser(false);
    CSSValue size_array = parser.ParseBackgroundSize();
    EXPECT_TRUE(size_array.IsEmpty());
  }

  {
    // Unknown keyword, no parse result.
    CSSParserConfigs configs;
    const char* val = "wrap";
    CSSStringParser parser{val, static_cast<uint32_t>(strlen(val)), configs};
    parser.SetIsLegacyParser(false);
    CSSValue size_array = parser.ParseBackgroundSize();
    EXPECT_TRUE(size_array.IsEmpty());
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

TEST(BackgroundSizeHandler, Valid) {
  CSSParserConfigs configs;
  const char* values[] = {"1px",     "1px auto",  "2% 3%",
                          "auto",    "auto auto", "auto 4%",
                          "contain", "cover",     "1px 2px, 3px 4px",
                          nullptr};

  constexpr int num_val = sizeof(values) / sizeof(char*) - 1;

  // check value equals value in target value array
  auto test_background_size_valid =
      [configs](const char* values[num_val],
                fml::RefPtr<lepus::CArray> expected_size[num_val]) {
        // For each testing values
        for (int i = 0; i < num_val; i++) {
          // Get parsed size value
          CSSStringParser parser{
              values[i], static_cast<uint32_t>(strlen(values[i])), configs};
          parser.SetIsLegacyParser(false);
          CSSValue size = parser.ParseBackgroundSize();
          // parsed result should be an array of <size-array>. And <size-array>
          // contains 4 value,( unit x, value x, unit y, value y)
          EXPECT_TRUE(size.IsArray());
          EXPECT_NE(size.GetValue().Array()->size(), 0);
          fml::RefPtr<lepus::CArray> size_array = size.GetValue().Array();
          EXPECT_EQ(size_array->size(), expected_size[i]->size());
          CheckLepusArrayNumberValue(size_array, expected_size[i]);
        }
      };

#pragma region BUILD_EXPECTED_SIZE
  fml::RefPtr<lepus::CArray> expected_size[9];

  typedef struct Size {
    uint32_t uw;
    double w;
    uint32_t uh;
    double h;
  } Size;

#define make_a_size(uw, w, uh, h)                     \
  (Size) {                                            \
    static_cast<uint32_t>(CSSValuePattern::uw), w,    \
        static_cast<uint32_t>(CSSValuePattern::uh), h \
  }
  // help function to create lepus array from value def.
  auto create_size_array = [](int len,
                              Size* sizes) -> fml::RefPtr<lepus::CArray> {
    auto array = lepus::CArray::Create();

    for (int i = 0; i < len; i++) {
      const auto [ux, vx, uy, vy] = *(sizes + i);
      auto value = lepus::CArray::Create();
      value->push_back(lepus::Value(ux));
      value->push_back(lepus::Value(vx));
      value->push_back(lepus::Value(uy));
      value->push_back(lepus::Value(vy));
      array->push_back(lepus::Value(value));
    }
    return array;
  };

  // 1px
  Size size = make_a_size(
      PX, 1.0, NUMBER,
      -1.0 * static_cast<int>(starlight::BackgroundSizeType::kAuto));

  expected_size[0] = create_size_array(1, &size);

  // 1px auto
  size = make_a_size(
      PX, 1, NUMBER,
      -1.0 * static_cast<int>(starlight::BackgroundSizeType::kAuto));
  expected_size[1] = create_size_array(1, &size);

  // 2% 3%
  size = make_a_size(PERCENT, 2, PERCENT, 3);
  expected_size[2] = create_size_array(1, &size);

  // auto
  size = make_a_size(
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kAuto),
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kAuto));
  expected_size[3] = create_size_array(1, &size);

  // auto auto
  size = make_a_size(
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kAuto),
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kAuto));
  expected_size[4] = create_size_array(1, &size);

  // auto 4%
  size = make_a_size(
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kAuto),
      PERCENT, 4);
  expected_size[5] = create_size_array(1, &size);

  // contain
  size = make_a_size(
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kContain),
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kContain));
  expected_size[6] = create_size_array(1, &size);

  // cover
  size = make_a_size(
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kCover),
      NUMBER, -1.0 * static_cast<int>(starlight::BackgroundSizeType::kCover));
  expected_size[7] = create_size_array(1, &size);

  // 1px 2px, 3px 4px
  Size sizes[] = {make_a_size(PX, 1, PX, 2), make_a_size(PX, 3, PX, 4)};
  expected_size[8] = create_size_array(2, sizes);
#pragma endregion BUILD_EXPECTED_SIZE
  test_background_size_valid(values, expected_size);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
