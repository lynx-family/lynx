// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>
#include <vector>

#include "core/renderer/css/css_color.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
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

struct ImageOrGradient {
  uint32_t type;
  std::string url;
  float angle;
  std::vector<uint32_t> color;
  std::vector<float> stop;

  explicit ImageOrGradient(const std::string& str)
      : type(static_cast<uint32_t>(starlight::BackgroundImageType::kUrl)),
        url(str),
        angle(0.f) {}

  ImageOrGradient(float angle, std::vector<uint32_t> const& c,
                  std::vector<float> const& s)
      : type(static_cast<uint32_t>(
            starlight::BackgroundImageType::kLinearGradient)),
        color(c),
        stop(s) {}

  void CheckValue(lepus::CArray* array, size_t index) const {
    ASSERT_TRUE(array->size() > index);

    auto value = array->get(index);
    ASSERT_TRUE(value.IsUInt32());
    switch (value.UInt32()) {
      case static_cast<uint32_t>(starlight::BackgroundImageType::kUrl):
        ASSERT_TRUE(array->size() > index + 1);
        ASSERT_TRUE(array->get(index + 1).IsString());
        ASSERT_EQ(url, array->get(index + 1).StdString());
        break;
      case static_cast<uint32_t>(
          starlight::BackgroundImageType::kLinearGradient):
        ASSERT_TRUE(array->size() > index + 1);
        ASSERT_TRUE(array->get(index + 1).IsArray());
        auto ga = array->get(index + 1).Array();
        CheckLinearGradient(ga.get());
        break;
    }
  }
  void CheckLinearGradient(lepus::CArray* array) const {
    if (stop.empty()) {
      LepusCheckEach(lepus::Value(angle), color);
    } else {
      LepusCheckEach(lepus::Value(angle), color, stop);
    }
  }
};

template <>
struct LepusCheck<std::vector<ImageOrGradient>> {
  static void Check(lepus::Value const& value,
                    std::vector<ImageOrGradient> const& t) {
    ASSERT_TRUE(value.IsArray());
    auto array = value.Array();
    size_t image_index = 0;
    for (size_t i = 0; i < array->size(); i++) {
      ASSERT_TRUE(array->get(i).IsUInt32());
      ASSERT_TRUE(t.size() > image_index);
      switch (array->get(i).UInt32()) {
        case static_cast<uint32_t>(starlight::BackgroundImageType::kNone):
          // TODO parse this
          continue;
        default:
          t[image_index++].CheckValue(array.get(), i++);
          break;
      }
    }
  }
};

using BGPosition = std::array<std::pair<uint32_t, float>, 2>;
using BGSize = BGPosition;
using BGRepeat = std::array<uint32_t, 2>;

template <>
struct LepusCheck<std::array<std::pair<uint32_t, float>, 2>> {
  using value_type = std::array<std::pair<uint32_t, float>, 2>;
  static void Check(lepus::Value const& value, value_type const& t) {
    LepusCheckEach(value, t[0].first, t[0].second, t[1].first, t[1].second);
  }
};

template <>
struct LepusCheck<std::array<uint32_t, 2>> {
  using value_type = std::array<uint32_t, 2>;

  static void Check(lepus::Value const& value, value_type const& t) {
    LepusCheckEach(value, t[0], t[1]);
  }
};

template <class T>
std::pair<uint32_t, float> MakeLengthT(T t, float value) {
  return std::make_pair(static_cast<uint32_t>(t), value);
}

template <class T>
BGRepeat MakeRepeat(T t1, T t2) {
  return BGRepeat{static_cast<uint32_t>(t1), static_cast<uint32_t>(t2)};
}

template <class T>
std::vector<uint32_t> MakeBoxList(T t) {
  std::vector<uint32_t> result;
  result.emplace_back(static_cast<uint32_t>(t));
  return result;
}

template <class T, class... Args>
std::vector<uint32_t> MakeBoxList(T t, Args&&... args) {
  std::vector<uint32_t> result;
  result.emplace_back(static_cast<uint32_t>(t));
  auto sub_list = MakeBoxList(args...);
  result.insert(result.end(), sub_list.begin(), sub_list.end());
  return result;
}

static void check_bg_array(const lepus::Value& value, uint32_t color,
                           std::vector<ImageOrGradient> const& image,
                           std::vector<BGPosition> const& position,
                           std::vector<BGSize> const& size,
                           std::vector<BGRepeat> const& repeat,
                           std::vector<uint32_t> const& origin,
                           std::vector<uint32_t> const& clip) {
  LepusCheckEach(value, color, image, position, size, repeat, origin, clip);
}

TEST(BackgroundHandler, Normal) {
  std::string bg_str = "url('https://yyy/i/bg_flower.gif')";
  CSSParserConfigs configs;
  CSSStringParser parser{bg_str.c_str(), static_cast<uint32_t>(bg_str.size()),
                         configs};
  // Open full feature
  parser.SetIsLegacyParser(false);

  CSSValue result = parser.ParseBackgroundOrMask(false);

  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);

  check_bg_array(
      result.GetValue(), 0, {ImageOrGradient{"https://yyy/i/bg_flower.gif"}},
      // position
      {{MakeLengthT(tasm::CSSValuePattern::PERCENT, 0.f),
        MakeLengthT(tasm::CSSValuePattern::PERCENT, 0.f)}},
      // size
      {{MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto)),
        MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto))}},
      // repeat
      {
          MakeRepeat(starlight::BackgroundRepeatType::kRepeat,
                     starlight::BackgroundRepeatType::kRepeat),
      },
      MakeBoxList(starlight::BackgroundOriginType::kPaddingBox),
      MakeBoxList(starlight::BackgroundOriginType::kPaddingBox));

  std::string bg_str_complex =
      "url('https://yyy/i/bg_flower.gif') left 5% / 15% 60% "
      "repeat-x "
      ",url('https://xxxx/ee/lynx-home/static/img/"
      "zh-logo-color.7c750dd6.png') red";

  CSSStringParser parser2{bg_str_complex.c_str(),
                          static_cast<uint32_t>(bg_str_complex.size()),
                          configs};

  parser2.SetIsLegacyParser(false);
  CSSValue result2 = parser2.ParseBackgroundOrMask(false);

  EXPECT_EQ(result2.GetPattern(), CSSValuePattern::ARRAY);
  check_bg_array(
      result2.GetValue(), 0xffff0000,
      {ImageOrGradient{"https://yyy/i/bg_flower.gif"},
       ImageOrGradient{"https://xxxx/ee/lynx-home/static/img/"
                       "zh-logo-color.7c750dd6.png"}},
      // position
      {{MakeLengthT(
            starlight::BackgroundPositionType::kLeft,
            -1.f * static_cast<int>(starlight::BackgroundPositionType::kLeft)),
        MakeLengthT(tasm::CSSValuePattern::PERCENT, 5.f)},
       {MakeLengthT(tasm::CSSValuePattern::PERCENT, 0.f),
        MakeLengthT(tasm::CSSValuePattern::PERCENT, 0.f)}},
      // size
      {{MakeLengthT(tasm::CSSValuePattern::PERCENT, 15.f),
        MakeLengthT(tasm::CSSValuePattern::PERCENT, 60.f)},
       {MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto)),
        MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto))}},
      // repeat
      {MakeRepeat(starlight::BackgroundRepeatType::kRepeat,
                  starlight::BackgroundRepeatType::kNoRepeat),
       MakeRepeat(starlight::BackgroundRepeatType::kRepeat,
                  starlight::BackgroundRepeatType::kRepeat)},
      MakeBoxList(starlight::BackgroundOriginType::kPaddingBox,
                  starlight::BackgroundOriginType::kPaddingBox),
      MakeBoxList(starlight::BackgroundClipType::kPaddingBox,
                  starlight::BackgroundClipType::kPaddingBox));

  std::string color_any_where =
      "content-box center / contain no-repeat "
      "url(\"../../media/examples/logo.svg\")"
      ", content-box #eee border-box 35% url(\"../../media/examples/1.png\")";

  CSSStringParser parser3{color_any_where.c_str(),
                          static_cast<uint32_t>(color_any_where.size()),
                          configs};

  parser3.SetIsLegacyParser(false);
  CSSValue result3 = parser3.ParseBackgroundOrMask(false);

  EXPECT_EQ(result3.GetPattern(), CSSValuePattern::ARRAY);
  check_bg_array(
      result3.GetValue(), 0xffeeeeee,
      {ImageOrGradient{"../../media/examples/logo.svg"},
       ImageOrGradient{"../../media/examples/1.png"}},
      // position
      {{MakeLengthT(starlight::BackgroundPositionType::kCenter,
                    -1.f * static_cast<int>(
                               starlight::BackgroundPositionType::kCenter)),
        MakeLengthT(starlight::BackgroundPositionType::kCenter,
                    -1.f * static_cast<int>(
                               starlight::BackgroundPositionType::kCenter))},
       {MakeLengthT(tasm::CSSValuePattern::PERCENT, 35.f),
        MakeLengthT(starlight::BackgroundPositionType::kCenter,
                    -1.f * static_cast<int>(
                               starlight::BackgroundPositionType::kCenter))}},
      // size
      {{MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kContain)),
        MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kContain))},
       {MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto)),
        MakeLengthT(
            tasm::CSSValuePattern::NUMBER,
            -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto))}},
      // repeat
      {MakeRepeat(starlight::BackgroundRepeatType::kNoRepeat,
                  starlight::BackgroundRepeatType::kNoRepeat),
       MakeRepeat(starlight::BackgroundRepeatType::kRepeat,
                  starlight::BackgroundRepeatType::kRepeat)},
      MakeBoxList(starlight::BackgroundOriginType::kContentBox,
                  starlight::BackgroundOriginType::kContentBox),
      MakeBoxList(starlight::BackgroundClipType::kContentBox,
                  starlight::BackgroundClipType::kBorderBox));
}

TEST(BackgroundHandler, Invalid) {
  std::vector<std::string> cases = {
      "url(https://zzz/obj/eden-cn/ulojhpenuln/"
      "brand-shop-bg.png) 100% 100% / cover top no-repeat';",
      "hello",
      "hello red",
      "red red",
      "url(https://lf3-static) left hello",
      "red url('https://xxxx/ee/lynx-home/static/img/"
      "zh-logo-color.7c750dd6.png'), radial-gradient(#FF0000, #00FF00)",
      "url(https://lf3-static) url(https://lf3-static)",
      "url(https://lf3-static) 100% 100% 100%",
      "url(https://lf3-static) 100% 100% top",
      "url(https://lf3-static) repeat-x repeat-y",
      "url(https://lf3-static) repeat-x repeat",
  };
  CSSParserConfigs configs;
  CSSPropertyID id = CSSPropertyID::kPropertyIDBackground;
  for (const auto& s : cases) {
    StyleMap output;
    auto impl = lepus::Value(s);
    EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
    EXPECT_TRUE(output.empty());
  }
}

TEST(BackgroundHandler, None) {
  std::string none = "NONE";
  CSSParserConfigs configs;
  CSSStringParser parser{none.c_str(), static_cast<uint32_t>(none.size()),
                         configs};

  CSSValue result = parser.ParseBackgroundOrMask(false);
  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
  EXPECT_EQ(result.GetValue().Array()->size(), 2);
  EXPECT_EQ(result.GetValue().Array()->get(0).Number(), 0);  // background-color
  const auto& images = result.GetValue().Array()->get(1);
  EXPECT_TRUE(images.IsArray());  // background-image is an array
  EXPECT_EQ(images.Array()->get(0).Number(),
            static_cast<int>(starlight::BackgroundImageType::kNone));  // none
}

TEST(BackgroundHandler, Color) {
  std::string red = "red";
  CSSParserConfigs configs;
  CSSStringParser parser{red.c_str(), static_cast<uint32_t>(red.size()),
                         configs};

  CSSValue result = parser.ParseBackgroundOrMask(false);
  EXPECT_EQ(result.GetPattern(), CSSValuePattern::ARRAY);
  EXPECT_EQ(result.GetValue().Array()->size(), 2);
  EXPECT_EQ(result.GetValue().Array()->get(0).Number(),
            0xffff0000);  // background-color
  const auto& images = result.GetValue().Array()->get(1);
  EXPECT_TRUE(images.IsArray());
  EXPECT_EQ(images.Array()->size(), 0);  // background-image is empty
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
