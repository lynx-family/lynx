// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_PARSER_CSS_STRING_PARSER_H_
#define CORE_RENDERER_CSS_PARSER_CSS_STRING_PARSER_H_

#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include "base/include/vector.h"
#include "core/renderer/css/css_keywords.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/css/parser/css_string_scanner.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {

/// A Recursive descent parser to parse CSS background and border string
/// more info see https://en.wikipedia.org/wiki/Recursive_descent_parser
/// Syntax is follow https://developer.mozilla.org/en-US/docs/Web/CSS/

static constexpr unsigned int POS_LEFT =
    static_cast<uint32_t>(starlight::BackgroundPositionType::kLeft);
static constexpr unsigned int POS_TOP =
    static_cast<uint32_t>(starlight::BackgroundPositionType::kTop);
static constexpr unsigned int POS_RIGHT =
    static_cast<uint32_t>(starlight::BackgroundPositionType::kRight);
static constexpr unsigned int POS_BOTTOM =
    static_cast<uint32_t>(starlight::BackgroundPositionType::kBottom);
static constexpr unsigned int POS_CENTER =
    static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter);
static constexpr uint32_t PATTERN_PERCENT =
    static_cast<uint32_t>(CSSValuePattern::PERCENT);
static constexpr int SIZE_AUTO =
    -1.f * static_cast<int>(starlight::BackgroundSizeType::kAuto);

class CSSStringParser final {
  struct StackValue {
    explicit StackValue(TokenType type) : value_type(type) {}
    StackValue(const lepus::Value& value, TokenType type)
        : value(value), value_type(type) {}
    StackValue() = default;

    // Inplace construct value
    template <typename... Args>
    explicit StackValue(TokenType type, Args&&... args)
        : value(std::forward<Args>(args)...), value_type(type) {}

    std::optional<lepus::Value> value;
    TokenType value_type;
    bool has_value{false};
  };

  struct CSSBackgroundLayer {
    CSSValue position_x;
    CSSValue position_y;
    // [-enum, number] | length
    CSSValue size_x;
    CSSValue size_y;
    uint32_t repeat_x =
        static_cast<uint32_t>(starlight::BackgroundRepeatType::kRepeat);
    uint32_t repeat_y =
        static_cast<uint32_t>(starlight::BackgroundRepeatType::kRepeat);
    uint32_t origin =
        static_cast<uint32_t>(starlight::BackgroundOriginType::kPaddingBox);
    uint32_t clip =
        static_cast<uint32_t>(starlight::BackgroundClipType::kPaddingBox);

    std::optional<StackValue> image;
    std::optional<uint32_t> color;

    CSSBackgroundLayer() {
      position_x.SetNumber(0, CSSValuePattern::PERCENT);
      position_y.SetNumber(0, CSSValuePattern::PERCENT);
      size_x.SetNumber(SIZE_AUTO, CSSValuePattern::NUMBER);
      size_y.SetNumber(SIZE_AUTO, CSSValuePattern::NUMBER);
    }
  };

  struct CSSTransitionLayer {
    double delay = 0;
    double duration = 0;
    starlight::AnimationPropertyType property =
        starlight::AnimationPropertyType::kAll;
    CSSValue timing_function;

    // The default value of Lynx's timing function is linear
    CSSTransitionLayer() {
      timing_function.SetEnum(
          static_cast<int>(starlight::TimingFunctionType::kLinear));
    }
  };

  struct CSSAnimationLayer {
    std::string name = "none";
    double delay = 0;
    double duration = 0;
    double count = 1;
    starlight::AnimationDirectionType direction =
        starlight::AnimationDirectionType::kNormal;
    starlight::AnimationFillModeType fill_mode =
        starlight::AnimationFillModeType::kNone;
    starlight::AnimationPlayStateType play_state =
        starlight::AnimationPlayStateType::kRunning;
    CSSValue timing_function;

    // The default value of Lynx's timing function is linear
    CSSAnimationLayer() {
      timing_function.SetEnum(
          static_cast<int>(starlight::TimingFunctionType::kLinear));
    }
  };

  enum {
    BG_REPEAT = 1 << 0,
    BG_POSITION_AND_SIZE = 1 << 1,
    BG_IMAGE = 1 << 2,
    BG_CLIP_BOX = 1 << 3,
    BG_ORIGIN = 1 << 4,
    BG_COLOR = 1 << 5,
  };

 public:
  CSSStringParser(const char* str, uint32_t len,
                  const CSSParserConfigs& configs)
      : scanner_(str, len), parser_configs_(configs) {}
  ~CSSStringParser() = default;

  static CSSStringParser FromLepusString(const lepus::Value& value_str,
                                         const CSSParserConfigs& configs) {
    auto str = value_str.StringView();
    return CSSStringParser(str.data(), static_cast<uint32_t>(str.size()),
                           configs);
  }

  /// <background> = [ <bg-layer>, ]* <final-bg-layer>
  CSSValue ParseBackgroundOrMask(bool mask);
  /// <bg-image> [, <bg-image> ]*
  CSSValue ParseBackgroundImage();

  /// <bg-position> [, <bg-position>]*
  /// <bg-position> =
  ///     [
  ///      left | center | right | top | bottom | <length-percentage> ]
  ///       | [ left | center | right | <length-percentage>
  ///     ]
  ///   [top | center | bottom | <length-percentage> ]
  CSSValue ParseBackgroundPosition();
  /// <bg-size> [, <bg-size> ]*
  /// <bg-size> = [ <length-percentage> | auto ]{1,2} | cover | contain
  CSSValue ParseBackgroundSize();
  /// <bg-origin>/<bg-clip> = <box> [, <box> ]*
  CSSValue ParseBackgroundBox();
  /// <bg-repeat> [, <bg-repeat>]*
  /// <bg-repeat> = <repeat-style> = repeat-x | repeat-y | [ repeat | space |
  ///               round | no-repeat ]{1,2}
  CSSValue ParseBackgroundRepeat();
  /// <text-color> = <color> | <linear-gradient> | <radial-gradient>
  CSSValue ParseBool();

  CSSValue ParseTextColor();
  /// <color>
  CSSValue ParseCSSColor();

  void ParseTextColorTo(CSSValue& target);
  void ParseCSSColorTo(CSSValue& target);

  /// <text-decoration> = <text-decoration-line> || <text-decoration-style> ||
  /// <text-decoration-color>
  CSSValue ParseTextDecoration();

  /// <src> = [ <url> [ format( <string> ) ]? | local( <family-name> )
  CSSValue ParseFontSrc();

  /// font-weight [ normal | bold | <number [1, 1000]>{1, 2}
  CSSValue ParseFontWeight();

  /// <length>
  CSSValue ParseFontLength();

  /// [ [ <url> [ <x> <y> ]? , ]* [ auto | default | none | ... ] ]
  CSSValue ParseCursor();

  inline void SetIsLegacyParser(bool is_legacy) { legacy_parser_ = is_legacy; }

  /// for image related only composed with url
  std::string ParseUrl();

  // <basic-shape>
  lepus::Value ParseClipPath();

  CSSValue ParseLength();
  void ParseLengthTo(CSSValue& target);

  CSSValue ParseListGap();

  CSSValue ParseSingleBorderRadius();
  CSSValue ParseAspectRatio();
  bool ParseTextStroke(CSSValue& result_width, CSSValue& result_color);
  std::pair<CSSValue, CSSValue> ParseGap();
  bool ParseBorderRadius(CSSValue horizontal_radii[4],
                         CSSValue vertical_radii[4]);

  CSSValue ParseFilter();
  bool ParseBorderLineWidth(CSSValue& result_width);
  bool ParseBorderStyle(CSSValue& result_style);
  bool ParseBorder(CSSValue& result_width, CSSValue& result_style,
                   CSSValue& result_color);

  CSSValue ParseShadow(bool inset_and_spread);
  lepus::Value ParseSingleShadow(bool inset_and_spread);

  CSSValue ParseTransform();
  bool ParseAutoFontSize(CSSValue& is_auto_font_size,
                         CSSValue& auto_font_size_min_size,
                         CSSValue& auto_font_size_max_size,
                         CSSValue& auto_font_size_step_granularity);
  bool ParseAutoFontSizePresetSize(fml::RefPtr<lepus::CArray>& arr);
  bool ParseTransformParams(const Token& function_token,
                            fml::RefPtr<lepus::CArray>& arr);
  bool ConsumeMatrixNumbers(fml::RefPtr<lepus::CArray>& arr, int count);
  CSSValue ParseTransformOrigin();

  bool ParseFlex(double& flex_grow, double& flex_shrink, CSSValue& flex_basis);

  static int32_t TokenTypeToBorderStyle(TokenType token_type);

  static uint32_t LerpColor(uint32_t start_color, uint32_t end_color,
                            float start_pos, float end_pos, float current_pos);

  bool ParseTransitionProperty(bool single, CSSValue& ret);
  bool ParseTime(bool single, bool no_negative, CSSValue& ret);
  bool ParseTimingFunction(bool single, CSSValue& ret);
  bool ParseTransition(bool single, lepus::Value arr[4]);
  bool ParseAnimationDirection(bool single, CSSValue& ret);
  bool ParseAnimationFillMode(bool single, CSSValue& ret);
  bool ParseAnimationIterCount(bool single, CSSValue& ret);
  bool ParseAnimationPlayState(bool single, CSSValue& ret);
  bool ParseAnimationName(bool single, CSSValue& ret);
  bool ParseAnimation(bool single, lepus::Value arr[8]);

  const char* content() const { return scanner_.content(); }

  template <auto F, typename... Args>
  bool ParseSingleOrMultipleValuePreview(Args&&... args) {
    bool single = std::strchr(content(), ',') == nullptr;
    return (this->*F)(single, std::forward<Args>(args)...);
  }

 private:
  template <typename TokenFunc, typename ConsumeTokenFunc>
  bool ParseNumberOrArray(bool single, TokenFunc is_token,
                          ConsumeTokenFunc consume, CSSValue& ret);
  bool Transition(CSSTransitionLayer& layer);
  bool Animation(CSSAnimationLayer& layer);

  bool AnimationDirectionValue(Token& token);
  bool AnimationFillModeValue(Token& token);
  bool AnimationIterCountValue(Token& token);
  bool AnimationPlayStateValue(Token& token);
  bool AnimationNameValue(Token& token);

  CSSValue TokenToLength(const Token& token);
  void TokenToLengthTarget(const Token& token, CSSValue& target);

  bool AtEnd();
  bool ConsumePosition(bool& horizontal_edge, bool& vertical_edge,
                       CSSValue& ret);
  /// LengthOrPercentage
  CSSValue Length();
  void LengthTo(CSSValue& target);

  /// NumberOrPercentage
  lepus::Value NumberOrPercentage();
  /// NumberOnly
  lepus::Value NumberOnly(bool nonnegative);
  /// <bg-layer> =
  ///  <bg-image> || <bg-position> [ / <bg-size> ]? || <repeat-style> || <box>
  ///  || <box>
  bool BackgroundLayer(CSSBackgroundLayer& layer, bool mask);
  /// <bg-image> =
  ///   none |
  ///   <url> |
  ///   <gradient>
  bool BackgroundImage();
  /// <bg-origin-box>/<bg-clip-box> = <box>
  lepus::Value BackgroundBox();
  /// <box> = [ border-box | padding-box | content-box ]
  bool Box(Token& token);
  /// <bg-position-and-size> = <bg-position> [ / <bg-size>] ?
  bool BackgroundPositionAndSize(CSSBackgroundLayer& layer);
  /// <bg-position> = [
  ///               [ left | center | right | top | bottom | <length-percentage>
  ///               ]
  ///               |
  ///               [ left | center | right | <length-percentage> ]  [ top |
  ///               center | bottom | <length-percentage> ]
  ///               |
  ///               [ center | [ left | right ] <length-percentage> ? ] && [
  ///               center | [ top | bottom ] < length-percentage> ? ]
  ///             ]
  bool BackgroundPosition(CSSValue& x, CSSValue& y);
  /// <bg-size> = [ <length-percentage> | auto ] {1, 2} | cover | contain
  bool BackgroundSize(CSSValue& x, CSSValue& y);
  /// <repeat-style> = repeat-x | repeat-y | [ repeat | no-repeat] {1, 2}
  bool BackgroundRepeatStyle(uint32_t& x, uint32_t& y);
  /// <text-decoration-line> = none | [underline || line-through]
  bool TextDecorationLine();
  /// <text-decoration-style> = solid | double | dotted | dashed | wavy
  bool TextDecorationStyle();
  /// <format> = format('<string>')
  bool Format();
  /// <local> = local('<string>')
  bool Local();
  /// <url> = url('<string>')
  bool Url();
  /// <gradient> =
  ///   <linear-gradient> |
  ///   <radial-gradient>
  bool Gradient();
  /// <linear-gradient> =linear-gradient ( [ <angle> | to <side-or-corner> ] ? ,
  /// <color-stop-list>)
  bool LinearGradient();
  /// <radial-gradient> = radial-gradient( [ <ending-shape> || <size> ] ? [ at
  /// <position> ] ?, <color-stop-list>)
  bool RadialGradient();
  /// <ending-shape> = ellipse | circle
  bool EndingShape();
  /// <size> = closest-side | closest-corner | farthest-side | farthest-corner
  bool EndingShapeSizeIdent();
  /// <color-stop-list> = [<color> [, <percentage>] ?]*
  bool ColorStopList(const fml::RefPtr<lepus::CArray>& colors,
                     const fml::RefPtr<lepus::CArray>& stops);

  /// <angle> = <number> [ deg | grad | rad | turn]
  bool AngleValue(Token& token);
  // <time> = <number> [ ms | s]
  bool TimeValue(Token& token);
  bool TransitionProperty(Token& token);
  bool TimingFunctionValue(Token& token);
  bool ParseTimingFunctionParams(const Token& function_token,
                                 fml::RefPtr<lepus::CArray>& arr);
  void ConsumeBorderLineWidth(Token& token, CSSValue& result);
  bool BorderWidthIdent(Token& token);
  bool BorderStyleIdent(Token& token);
  bool TransformFunctionIdent(Token& token);

  void ConsumeColor(CSSValue& result);
  bool ShadowOptionIdent(Token& token);
  template <typename Func, typename... Args>
  CSSValue ConsumeCommaSeparatedList(Func callback, Args&&... args);
  /// <color> =
  ///   <rgba()> |
  ///   <rgb()> |
  ///   <hsla()> |
  ///   <hsl()> |
  ///   <hex-color> |
  ///   <named-color>
  bool Color();
  /// <rgba()> = rgba( <number> , <number>, <number> , <alpha-value>)
  bool RGBAColor();
  /// <rgb()> = rgb(<number> , <number> , <number>)
  bool RGBColor();
  /// <hsla()> = hsla( <number> | <angle>, <percentage>, <percentage>
  bool HSLAColor();
  bool HSLColor();
  /// <hex-color> = #<number>
  bool HexColor();
  /// <number/percentage-value> = ( <percentage-value> | <number>)
  bool NumberOrPercentValue(Token& token);
  /// <percentage-value> = <number> %
  bool PercentageValue(Token& token);
  bool DimensionValue(Token& token);
  bool NumberValue(Token& token);
  bool HexValue(Token& token);
  /// <basic-shape> =
  ///   inset( <shape-arg>{1,4} [round <border-radius>]? ) |
  ///   circle( [<shape-radius>] [at <position>]? ) |
  ///   ellipse( [<shape-radius>{2}]  [at <position>]? ) |
  ///   polygon( [<fill-rule>,]  [<shape-arg> <shape-arg>]# ) |
  ///   path( [<fill-rule>,]? <string>)
  bool BasicShape();
  /// circle( [shape-radius]? [at <position>]?)
  bool BasicShapeCircle();
  /// ellipse([<shape-radius>{2}]? [at <position>]?)
  bool BasicShapeEllipse();
  bool ConsumeLengthAndSetValue(fml::RefPtr<lepus::CArray>& arr);
  bool AtPositionAndSetValue(fml::RefPtr<lepus::CArray>&);
  bool ConsumePositionAndSetValue(fml::RefPtr<lepus::CArray>&);
  /// path(<string>)
  /// a string of SVG path data follow EBNF grammar
  bool BasicShapePath();
  /// super-ellipse([<shape-radius>{2}] [<number>{2}] [at <position>] ?)
  bool SuperEllipse();

  /// inset([<length-percentage>{1,4} [(round | super-ellipse ex ey)
  /// <border-radius>]?])
  bool BasicShapeInset();

  /// <length-percentage>{1,4} [/ <length-percentage>{1,4}]?
  bool BorderRadius(CSSValue horizontal_radii[4], CSSValue vertical_radii[4]);

  void PushValue(const StackValue& value);
  void PushValue(StackValue&& value);
  const StackValue& PopValue();

  // Scanner function
  bool CheckAndAdvance(TokenType type);
  bool Consume(TokenType type);
  bool Check(TokenType type);
  void Advance();

  // utils function
  static uint32_t TokenTypeToTextENUM(TokenType token_type);
  static uint32_t TokenTypeToENUM(TokenType token_type);
  static uint32_t TokenTypeToBorderWidth(TokenType token_type);
  static int TokenTypeToShadowOption(TokenType token_type);
  static double GetColorValue(const Token& token, double max_value = 255);
  static StackValue MakeColorValue(const Token token_list[]);
  static int64_t TokenToInt(const Token& token);
  static double TokenToDouble(const Token& token);
  static float TokenToAngleValue(const Token& token);
  static double TimeToNumber(const Token& token);
  static starlight::TransformType TokenToTransformFunction(const Token& token);
  static starlight::AnimationPropertyType TokenToTransitionType(
      const Token& token, const CSSParserConfigs& configs);
  static starlight::TimingFunctionType TokenToTimingFunctionType(
      const Token& token);
  static starlight::AnimationDirectionType TokenToAnimationDirectionType(
      const Token& token);
  static starlight::AnimationFillModeType TokenToAnimationFillModeType(
      const Token& token);
  static double TokenToAnimationIterCount(const Token& token);
  static starlight::AnimationPlayStateType TokenToAnimationPlayState(
      const Token& token);
  static CSSValue ConsumeTimingFunction(const Token& token,
                                        const CSSParserConfigs& configs);
  static void BackgroundLayerToArray(const CSSBackgroundLayer& layer,
                                     lepus::CArray* image_array,
                                     lepus::CArray* position_array,
                                     lepus::CArray* size_array,
                                     lepus::CArray* origin_array,
                                     lepus::CArray* repeat_array,
                                     lepus::CArray* clip_array);
  static void ClampColorAndStopList(base::Vector<uint32_t>& colors,
                                    base::Vector<float>& stops);
  static void ClampColorAndStopListAtFront(base::Vector<uint32_t>& colors,
                                           base::Vector<float>& stops,
                                           uint32_t first_positive_index);
  static void ClampColorAndStopListAtBack(base::Vector<uint32_t>& colors,
                                          base::Vector<float>& stops,
                                          uint32_t tail_position);
  void SkipWhitespaceToken();
  bool ConsumeAndSave(TokenType tokenType, Token& token);
  bool LengthOrPercentageValue(Token& token);
  bool ConsumeGrayscale(Token& token);
  bool ConsumeBlur(Token& token);
  CSSValue ParseGrayscale();
  CSSValue ParseBlur();
  CSSValue FilterGrayscaleValue(const Token& function_token);
  CSSValue FilterBlurValue(const Token& function_token);

  StackValue stack_value_;
  Token current_token_;
  Token previous_token_;
  Scanner scanner_;
  bool legacy_parser_ = true;
  bool enable_transform_legacy_ = false;
  bool enable_time_legacy_ = false;
  CSSParserConfigs parser_configs_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_PARSER_CSS_STRING_PARSER_H_
