// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/background_image_handler.h"

#include "core/renderer/css/css_color.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

template <typename T, typename Enable = void>
struct LepusCheck {};

template <>
struct LepusCheck<uint32_t> {
  static void Check(lepus::Value const& value, uint32_t t) {
    ASSERT_TRUE(value.IsUInt32());
    EXPECT_EQ(value.UInt32(), t);
  }
};

template <>
struct LepusCheck<int32_t> {
  static void Check(lepus::Value const& value, int32_t t) {
    ASSERT_TRUE(value.IsNumber());
    EXPECT_EQ(static_cast<int32_t>(value.Number()), t);
  }
};

template <>
struct LepusCheck<float> {
  static void Check(lepus::Value const& value, float t) {
    ASSERT_TRUE(value.IsNumber());
    EXPECT_NEAR(value.Number(), t, 0.01);
  }
};

template <>
struct LepusCheck<std::string> {
  static void Check(lepus::Value const& value, std::string const& t) {
    ASSERT_TRUE(value.IsString());
    EXPECT_EQ(value.StdString(), t);
  }
};

template <>
struct LepusCheck<const char*> {
  static void Check(lepus::Value const& value, const char* t) {
    ASSERT_TRUE(value.IsString());
    EXPECT_EQ(value.StdString(), std::string(t));
  }
};

template <typename T>
struct LepusCheck<T, typename std::enable_if_t<std::is_enum<T>::value>> {
  static void Check(lepus::Value const& value, T t) {
    LepusCheck<uint32_t>::Check(value, static_cast<uint32_t>(t));
  }
};

template <typename T>
struct LepusCheck<std::vector<T>> {
  static void Check(lepus::Value const& value, std::vector<T> const& t) {
    ASSERT_TRUE(value.IsArray());
    auto array = value.Array();
    for (size_t i = 0; i < t.size(); i++) {
      LepusCheck<T>::Check(array->get(i), t[i]);
    }
  }
};

template <typename T, typename F, size_t... I>
void TupleForEach(T&& t, F f, std::integer_sequence<size_t, I...>) {
  auto l = {(f(I, std::get<I>(t)), 0)...};
  // prevent unused warnning.
  LOGI(&l);
}

template <typename T>
void LepusCheckFunc(lepus::Value const& value, T const& t) {
  LepusCheck<T>::Check(value, t);
}

template <typename... Args>
struct LepusCheck<std::tuple<Args...>> {
  using tuple_type = typename std::tuple<Args...>;
  static void Check(lepus::Value const& value, tuple_type const& t) {
    ASSERT_TRUE(value.IsArray());
    auto array = value.Array();
    ASSERT_EQ(array->size(), std::tuple_size<tuple_type>::value);

    TupleForEach(
        t, [array](size_t i, auto f) { LepusCheckFunc(array->get(i), f); },
        std::make_integer_sequence<size_t, sizeof...(Args)>());
  }
};

template <typename... Args>
void LepusCheckEach(lepus::Value const& value, Args&&... args) {
  std::tuple<Args...> tuple{args...};
  LepusCheck<std::tuple<Args...>>::Check(value, tuple);
}

TEST(BackgroundImageHandler, parse_url_image) {
  std::string url_list =
      "url('https://yyy/i/bg_flower.gif'), "
      "url('https://tttt/files/7693/catfront.png'),url('https:/"
      "/xxxx/ee/lynx-home/static/img/"
      "zh-logo-color.7c750dd6.png');";
  CSSParserConfigs configs;
  CSSStringParser url_list_parser{
      url_list.c_str(), static_cast<uint32_t>(url_list.size()), configs};

  CSSValue result = url_list_parser.ParseBackgroundImage();

  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
  EXPECT_TRUE(result.IsArray());
  auto url_array = result.GetValue().Array();
  EXPECT_EQ(url_array->size(), static_cast<size_t>(6));

  LepusCheckEach(result.GetValue(), starlight::BackgroundImageType::kUrl,
                 "https://yyy/i/bg_flower.gif",
                 starlight::BackgroundImageType::kUrl,
                 "https://tttt/files/7693/catfront.png",
                 starlight::BackgroundImageType::kUrl,
                 "https://xxxx/ee/lynx-home/static/img/"
                 "zh-logo-color.7c750dd6.png");

  std::string url_list2 = "url(\"https://yyy/i/bg_flower.gif\")";
  CSSStringParser url_list_parser2{
      url_list2.c_str(), static_cast<uint32_t>(url_list2.size()), configs};

  CSSValue result2 = url_list_parser2.ParseBackgroundImage();
  EXPECT_EQ(result2.GetPattern(), CSSValuePattern::ARRAY);
  LepusCheckEach(result2.GetValue(), starlight::BackgroundImageType::kUrl,
                 "https://yyy/i/bg_flower.gif");
}

TEST(BackgroundImageHandler, parse_url_data) {
  std::string url_list = "url(\"data:image/png;base64,\") ";
  CSSParserConfigs configs;
  CSSStringParser url_list_parser{
      url_list.c_str(), static_cast<uint32_t>(url_list.size()), configs};

  CSSValue result = url_list_parser.ParseBackgroundImage();

  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
  EXPECT_TRUE(result.IsArray());
  auto url_array = result.GetValue().Array();
  EXPECT_EQ(url_array->size(), static_cast<size_t>(2));

  LepusCheckEach(result.GetValue(), starlight::BackgroundImageType::kUrl,
                 "data:image/png;base64,");
}

static void ExpectLinearGradientEQ(const lepus::Value& value, float deg,
                                   const std::vector<uint32_t>& colors,
                                   const std::vector<float>& stops,
                                   int direction) {
  LepusCheckEach(value, deg, colors, stops, direction);
}

TEST(BackgroundImageHandler, parse_linear_gradient) {
  CSSParserConfigs configs;
  std::string bg_image = "linear-gradient(to left, red, blue 30%, green 0.9);";
  CSSStringParser parser{bg_image.c_str(),
                         static_cast<uint32_t>(bg_image.size()), configs};

  CSSValue result = parser.ParseBackgroundImage();

  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
  auto url_array = result.GetValue().Array();
  EXPECT_EQ(url_array->size(), static_cast<size_t>(2));
  EXPECT_TRUE(url_array->get(0).IsUInt32());
  EXPECT_EQ(
      url_array->get(0).UInt32(),
      static_cast<uint32_t>(starlight::BackgroundImageType::kLinearGradient));

  ExpectLinearGradientEQ(url_array->get(1), 270.f,
                         {0xffff0000, 0xff0000ff, 0xff008000},
                         {0.f, 30.f, 90.f}, 3);

  std::string bg_image2 =
      "linear-gradient(rgba(0, 0, 255, 0.5), rgba(255, 255, 0, 0.5));";
  CSSStringParser parser2{bg_image2.c_str(),
                          static_cast<uint32_t>(bg_image2.size()), configs};

  CSSValue value2 = parser2.ParseBackgroundImage();

  ASSERT_TRUE(value2.IsArray());
  LepusCheckEach(
      value2.GetValue(), starlight::BackgroundImageType::kLinearGradient,
      std::make_tuple(180.f,
                      std::vector<uint32_t>{CSSColor(0, 0, 255, 0.5f).Cast(),
                                            CSSColor{255, 255, 0, 0.5f}.Cast()},
                      std::vector<float>{}, 2));
}

TEST(BackgroundImageHandler, parse_radial_gradient) {
  std::string radial_gradient =
      "radial-gradient(ellipse at top, red, transparent)";
  CSSParserConfigs configs;
  CSSStringParser parser{radial_gradient.c_str(),
                         static_cast<uint32_t>(radial_gradient.size()),
                         configs};
  CSSValue value = parser.ParseBackgroundImage();

  ASSERT_TRUE(value.IsArray());
  auto array = value.GetValue().Array();
  LepusCheckFunc(array->get(0),
                 starlight::BackgroundImageType::kRadialGradient);
  LepusCheckEach(
      array->get(1),
      std::make_tuple(starlight::RadialGradientShapeType::kEllipse,
                      starlight::RadialGradientSizeType::kFarthestCorner,
                      starlight::BackgroundPositionType::kCenter,
                      -1.f * static_cast<uint32_t>(
                                 starlight::BackgroundPositionType::kCenter),
                      starlight::BackgroundPositionType::kTop, -32),
      std::vector<uint32_t>{0xffff0000, 0x0}, std::vector<float>{});
}

TEST(BackgroundImageHandler, parse_radial_gradient_with_array) {
  std::string radial_gradient =
      "radial-gradient(ellipse at top, red, transparent), "
      "radial-gradient(ellipse at right, blue, transparent)";
  CSSParserConfigs configs;
  CSSStringParser parser{radial_gradient.c_str(),
                         static_cast<uint32_t>(radial_gradient.size()),
                         configs};
  CSSValue value = parser.ParseBackgroundImage();

  ASSERT_TRUE(value.IsArray());
  auto array = value.GetValue().Array();
  LepusCheckFunc(array->get(0),
                 starlight::BackgroundImageType::kRadialGradient);
  LepusCheckEach(
      array->get(1),
      std::make_tuple(starlight::RadialGradientShapeType::kEllipse,
                      starlight::RadialGradientSizeType::kFarthestCorner,
                      starlight::BackgroundPositionType::kCenter,
                      -1.f * static_cast<uint32_t>(
                                 starlight::BackgroundPositionType::kCenter),
                      starlight::BackgroundPositionType::kTop,
                      -1.f * static_cast<uint32_t>(
                                 starlight::BackgroundPositionType::kTop)),
      std::vector<uint32_t>{0xffff0000, 0x0}, std::vector<float>{});
  LepusCheckFunc(array->get(2),
                 starlight::BackgroundImageType::kRadialGradient);
  LepusCheckEach(
      array->get(3),
      std::make_tuple(starlight::RadialGradientShapeType::kEllipse,
                      starlight::RadialGradientSizeType::kFarthestCorner,
                      starlight::BackgroundPositionType::kRight,
                      -1.f * static_cast<uint32_t>(
                                 starlight::BackgroundPositionType::kRight),
                      starlight::BackgroundPositionType::kCenter,
                      -1.f * static_cast<uint32_t>(
                                 starlight::BackgroundPositionType::kCenter)),
      std::vector<uint32_t>{0xff0000ff, 0x0}, std::vector<float>{});
}

TEST(BackgroundImageHandler, parse_radial_gradient_with_size) {
  std::string radial_gradient =
      "radial-gradient(ellipse 10px 5px at top, red, transparent)";
  CSSParserConfigs configs;
  CSSStringParser parser{radial_gradient.c_str(),
                         static_cast<uint32_t>(radial_gradient.size()),
                         configs};
  CSSValue value = parser.ParseBackgroundImage();

  ASSERT_TRUE(value.IsArray());
  auto array = value.GetValue().Array();
  LepusCheckFunc(array->get(0),
                 starlight::BackgroundImageType::kRadialGradient);
  LepusCheckEach(
      array->get(1),
      std::make_tuple(starlight::RadialGradientShapeType::kEllipse,
                      starlight::RadialGradientSizeType::kLength,
                      starlight::BackgroundPositionType::kCenter,
                      -1.f * static_cast<uint32_t>(
                                 starlight::BackgroundPositionType::kCenter),
                      starlight::BackgroundPositionType::kTop, -32,
                      CSSValuePattern::PX, 10, CSSValuePattern::PX, 5),
      std::vector<uint32_t>{0xffff0000, 0x0}, std::vector<float>{});
}

TEST(BackgroundImageHandler, parse_radial_gradient_with_circle) {
  std::string radial_gradient =
      "radial-gradient(circle 10px at top, red, transparent)";
  CSSParserConfigs configs;
  CSSStringParser parser{radial_gradient.c_str(),
                         static_cast<uint32_t>(radial_gradient.size()),
                         configs};
  CSSValue value = parser.ParseBackgroundImage();

  ASSERT_TRUE(value.IsArray());
  auto array = value.GetValue().Array();
  LepusCheckFunc(array->get(0),
                 starlight::BackgroundImageType::kRadialGradient);
  LepusCheckEach(
      array->get(1),
      std::make_tuple(starlight::RadialGradientShapeType::kCircle,
                      starlight::RadialGradientSizeType::kLength,
                      starlight::BackgroundPositionType::kCenter,
                      -1.f * static_cast<uint32_t>(
                                 starlight::BackgroundPositionType::kCenter),
                      starlight::BackgroundPositionType::kTop, -32,
                      CSSValuePattern::PX, 10, CSSValuePattern::PX, 10),
      std::vector<uint32_t>{0xffff0000, 0x0}, std::vector<float>{});
}

TEST(BackgroundImageHandler, parse_radial_gradient_with_shape_size) {
  std::string radial_gradients[] = {
      "radial-gradient(farthest-corner at center, red, transparent)",
      "radial-gradient(farthest-side at center, red, transparent)",
      "radial-gradient(closest-corner at center, red, transparent)",
      "radial-gradient(closest-side at center, red, transparent)",
  };

  for (int i = 0; i < std::size(radial_gradients); i++) {
    const auto& g = radial_gradients[i];
    CSSParserConfigs configs;
    CSSStringParser parser{g.c_str(), static_cast<uint32_t>(g.size()), configs};
    CSSValue value = parser.ParseBackgroundImage();

    auto array = value.GetValue().Array();
    LepusCheckFunc(array->get(0),
                   starlight::BackgroundImageType::kRadialGradient);
    LepusCheckEach(
        array->get(1),
        std::make_tuple(starlight::RadialGradientShapeType::kEllipse,
                        static_cast<starlight::RadialGradientSizeType>(i),
                        starlight::BackgroundPositionType::kCenter,
                        -1.f * static_cast<uint32_t>(
                                   starlight::BackgroundPositionType::kCenter),
                        starlight::BackgroundPositionType::kCenter,
                        -1.f * static_cast<uint32_t>(
                                   starlight::BackgroundPositionType::kCenter)),
        std::vector<uint32_t>{0xffff0000, 0x0}, std::vector<float>{});
  }
}

TEST(BackgroundImageHandler, parse_radial_gradient_with_sizes) {
  {
    // The shape should be circle
    std::string g1 = "radial-gradient(10px at top, red, transparent)";
    std::string g2 = "radial-gradient(circle 10px at top, red, transparent)";
    CSSParserConfigs configs;
    CSSStringParser p1{g1.c_str(), static_cast<uint32_t>(g2.size()), configs};
    CSSValue v1 = p1.ParseBackgroundImage();

    CSSStringParser p2{g2.c_str(), static_cast<uint32_t>(g2.size()), configs};
    CSSValue v2 = p2.ParseBackgroundImage();
    EXPECT_EQ(v1, v2);
  }

  {
    // The shape should be ellipse
    std::string g3 = "radial-gradient(10px 5px at top, red, transparent)";
    std::string g4 =
        "radial-gradient(ellipse 10px 5px at top, red, transparent)";
    CSSParserConfigs configs;
    CSSStringParser p3{g3.c_str(), static_cast<uint32_t>(g3.size()), configs};
    CSSValue v3 = p3.ParseBackgroundImage();

    CSSStringParser p4{g4.c_str(), static_cast<uint32_t>(g4.size()), configs};
    CSSValue v4 = p4.ParseBackgroundImage();
    EXPECT_EQ(v3, v4);
  }
}

TEST(BackgroundImageHandler, parse_radial_gradient_invalid) {
  std::string radial_gradients[] = {
      "radial-gradient(ellipse farthest-corner 10px at top, red, transparent)",
      "radial-gradient(circle 10px 5px at top, red, transparent)",
      "radial-gradient(ellipse 10px at top, red, transparent)",
      "radial-gradient(ellipse ellipse, red, transparent)",
      "radial-gradient(farthest-corner at center)",
  };

  for (const auto& g : radial_gradients) {
    SCOPED_TRACE(g);
    CSSParserConfigs configs;
    CSSStringParser parser{g.c_str(), static_cast<uint32_t>(g.size()), configs};
    CSSValue value = parser.ParseBackgroundImage();

    ASSERT_TRUE(value.IsEmpty());
  }
}

TEST(BackgroundImageHandler, parse_linear_gradient_option_stop) {
  std::string bg_image =
      "linear-gradient(to left, red, blue, green 0.9, blue, black 150%);";
  CSSParserConfigs configs;
  CSSStringParser parser{bg_image.c_str(),
                         static_cast<uint32_t>(bg_image.size()), configs};

  CSSValue result = parser.ParseBackgroundImage();

  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
  auto url_array = result.GetValue().Array();
  EXPECT_EQ(url_array->size(), static_cast<size_t>(2));
  EXPECT_TRUE(url_array->get(0).IsUInt32());
  EXPECT_EQ(
      url_array->get(0).UInt32(),
      static_cast<uint32_t>(starlight::BackgroundImageType::kLinearGradient));
  EXPECT_TRUE(url_array->get(1).IsArray());

  uint32_t red = 0xffff0000;
  uint32_t blue = 0xff0000ff;
  uint32_t green = 0xff008000;
  uint32_t mix = CSSStringParser::LerpColor(green, blue, 0.9f, 1.2, 1.f);

  ExpectLinearGradientEQ(url_array->get(1), 270.f, {red, blue, green, mix},
                         {0.f, 45.f, 90.f, 100.f}, 3);
}

TEST(BackgroundImageHandler, parse_linear_gradient_option_start) {
  std::string bg_image = "linear-gradient(to left, red -10%, blue 10%, green);";
  CSSParserConfigs configs;
  CSSStringParser parser{bg_image.c_str(),
                         static_cast<uint32_t>(bg_image.size()), configs};

  CSSValue result = parser.ParseBackgroundImage();

  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
  auto url_array = result.GetValue().Array();
  EXPECT_EQ(url_array->size(), static_cast<size_t>(2));
  EXPECT_TRUE(url_array->get(0).IsUInt32());
  EXPECT_EQ(
      url_array->get(0).UInt32(),
      static_cast<uint32_t>(starlight::BackgroundImageType::kLinearGradient));
  EXPECT_TRUE(url_array->get(1).IsArray());

  uint32_t red = 0xffff0000;
  uint32_t blue = 0xff0000ff;
  uint32_t green = 0xff008000;
  uint32_t mix = CSSStringParser::LerpColor(red, blue, -0.1f, 0.1f, 0.f);

  ExpectLinearGradientEQ(url_array->get(1), 270.f, {mix, blue, green},
                         {0.f, 10.f, 100.f}, 3);
}

TEST(BackgroundImageHandler, linear_gradient_angle_valid_value) {
  typedef struct LinearGradientAngleKV {
    const char* key;
    double value;
  } LinearGradientAngleKV;
  const LinearGradientAngleKV values[] = {
      {.key = "linear-gradient(green, green)", .value = 180.f},
      {.key = "linear-gradient(90DeG, green, green)", .value = 90.f},
      {.key = "linear-gradient(100gRaD, green, green)", .value = 90.f},
      {.key = "linear-gradient(1.57rAd, green, green)", .value = 89.954373f},
      {.key = "linear-gradient(0.25tUrN, green, green)", .value = 90.f},
      {nullptr}};
  CSSParserConfigs configs;
  for (const LinearGradientAngleKV* it = values; it->key; it++) {
    const char* key = it->key;
    CSSStringParser parser{key, static_cast<uint32_t>(strlen(key)), configs};
    CSSValue result = parser.ParseBackgroundImage();
    EXPECT_TRUE(result.IsArray());
    EXPECT_EQ(
        result.GetValue().Array()->get(0).Number(),
        static_cast<uint32_t>(starlight::BackgroundImageType::kLinearGradient));
    lepus::CArray& gradient_array =
        *(result.GetValue().Array()->get(1).Array().get());
    EXPECT_FLOAT_EQ(gradient_array.get(0).Number(), it->value);
  }
}

TEST(BackgroundImageHandler, linear_gradient_value_invalid) {
  const char* invalid_values[] = {"linear-gradient(90degree, red, red)",
                                  "linear-gradient(100gradian, red, red)",
                                  "linear-gradient(1.57radian, red, red)",
                                  "linear-gradient(0.25turns, red, red)",
                                  nullptr};
  CSSParserConfigs configs;
  for (const char** it = invalid_values; *it; it++) {
    CSSStringParser parser{*it, static_cast<uint32_t>(strlen(*it)), configs};
    CSSValue gradient = parser.ParseBackgroundImage();
    EXPECT_TRUE(gradient.IsEmpty());
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
