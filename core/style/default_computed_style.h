// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_DEFAULT_COMPUTED_STYLE_H_
#define CORE_STYLE_DEFAULT_COMPUTED_STYLE_H_

#include <vector>

#include "base/include/no_destructor.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/animation_data.h"
#include "core/style/shadow_data.h"
#include "core/style/transform_origin_data.h"

namespace lynx {
namespace starlight {

struct DefaultComputedStyle {
  static constexpr int DEFAULT_TEXT_MAX_LINE = -1;
  static constexpr int DEFAULT_TEXT_MAX_LENGTH = -1;

  static constexpr float DEFAULT_LINE_HEIGHT = -1.0f;
  static constexpr float DEFAULT_LINE_HEIGHT_FACTOR = -1.0f;
  static constexpr float DEFAULT_LINE_SPACING = -1.0f;
  static constexpr float DEFAULT_LETTER_SPACING = -1.0f;
  static constexpr float DEFAULT_FLOAT = 0.0f;
  static constexpr float DEFAULT_OPACITY = 1.0f;

  static constexpr bool DEFAULT_AUTO_FONT_SIZE = false;
  static constexpr float DEFAULT_AUTO_FONT_SIZE_STEP_GRANULARITY = 1.0f;

  static constexpr TextAlignType DEFAULT_TEXT_ALIGN = TextAlignType::kStart;
  static constexpr TextOverflowType DEFAULT_TEXT_OVERFLOW =
      TextOverflowType::kClip;

  static constexpr long DEFAULT_LONG = 0;
  static constexpr bool DEFAULT_BOOLEAN = false;

  static constexpr FontWeightType DEFAULT_FONT_WEIGHT = FontWeightType::kNormal;
  static constexpr FontStyleType DEFAULT_FONT_STYLE = FontStyleType::kNormal;

  static constexpr OverflowType DEFAULT_OVERFLOW = OverflowType::kHidden;

  static constexpr WhiteSpaceType DEFAULT_WHITE_SPACE = WhiteSpaceType::kNormal;
  static constexpr WordBreakType DEFAULT_WORD_BREAK = WordBreakType::kNormal;
  static constexpr BorderStyleType DEFAULT_OUTLINE_STYLE =
      BorderStyleType::kNone;
  static constexpr VisibilityType DEFAULT_VISIBILITY = VisibilityType::kVisible;
  static constexpr VerticalAlignType DEFAULT_VERTICAL_ALIGN =
      VerticalAlignType::kDefault;
  static constexpr HyphensType DEFAULT_HYPHENS = HyphensType::kManual;
  static constexpr XAnimationColorInterpolationType DEFAULT_INTERPOLATION_TYPE =
      XAnimationColorInterpolationType::kAuto;
  static constexpr uint32_t DEFAULT_TEXT_DECORATION_STYLE =
      static_cast<uint32_t>(TextDecorationType::kSolid);
  static std::vector<float> DEFAULT_AUTO_FONT_SIZE_PRESET_SIZES() {
    static base::NoDestructor<std::vector<float>> l{std::vector<float>()};
    return *l;
  }

  static AnimationData DEFAULT_ANIMATION() {
    static base::NoDestructor<AnimationData> l{AnimationData()};
    return *l;
  }

  static std::vector<ShadowData> DEFAULT_BOX_SHADOW() {
    static base::NoDestructor<std::vector<ShadowData>> l{
        std::vector<ShadowData>()};
    return *l;
  }

  static TransformOriginData DEFAULT_TRANSFORM_ORIGIN() {
    static base::NoDestructor<TransformOriginData> l{TransformOriginData()};
    return *l;
  }
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_DEFAULT_COMPUTED_STYLE_H_
