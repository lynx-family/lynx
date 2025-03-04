// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_DEFAULT_LAYOUT_STYLE_H_
#define CORE_RENDERER_STARLIGHT_STYLE_DEFAULT_LAYOUT_STYLE_H_

#include <vector>

#include "base/include/no_destructor.h"
#include "core/renderer/starlight/style/box_data.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/style/flex_data.h"
#include "core/renderer/starlight/style/linear_data.h"
#include "core/renderer/starlight/style/relative_data.h"

#define CSS_UNDEFINED 0x7FFFFFF
static constexpr float UNDEFINED = 10E20;

#define DEFAULT_CSS_FUNC(type, name)               \
  (type ? DefaultLayoutStyle::W3C_DEFAULT_##name() \
        : DefaultLayoutStyle::SL_DEFAULT_##name())

#define DEFAULT_CSS_VALUE(type, name)            \
  (type ? DefaultLayoutStyle::W3C_DEFAULT_##name \
        : DefaultLayoutStyle::SL_DEFAULT_##name)

namespace lynx {
namespace starlight {

struct DefaultLayoutStyle {
  static constexpr int SL_DEFAULT_RELATIVE_ID = -1;
  static constexpr int SL_DEFAULT_RELATIVE_ALIGN_TOP = -1;
  static constexpr int SL_DEFAULT_RELATIVE_ALIGN_RIGHT = -1;
  static constexpr int SL_DEFAULT_RELATIVE_ALIGN_BOTTOM = -1;
  static constexpr int SL_DEFAULT_RELATIVE_ALIGN_LEFT = -1;
  static constexpr int SL_DEFAULT_RELATIVE_TOP_OF = -1;
  static constexpr int SL_DEFAULT_RELATIVE_RIGHT_OF = -1;
  static constexpr int SL_DEFAULT_RELATIVE_BOTTOM_OF = -1;
  static constexpr int SL_DEFAULT_RELATIVE_LEFT_OF = -1;

  static constexpr int32_t SL_DEFAULT_GRID_SPAN = 1;
  static constexpr int32_t SL_DEFAULT_GRID_ITEM_POSITION = 0;

  static constexpr float kDefaultMinSize = 0.f;
  static constexpr float kDefaultMaxSize = static_cast<float>(CSS_UNDEFINED);
  static constexpr float SL_DEFAULT_FLEX_GROW = 0.0f;
  static constexpr float SL_DEFAULT_FLEX_SHRINK = 1.0f;
  static constexpr float SL_DEFAULT_ORDER = 0.0f;
  static constexpr float SL_DEFAULT_LINEAR_WEIGHT_SUM = 0.0f;
  static constexpr float SL_DEFAULT_LINEAR_WEIGHT = 0.0f;
  static constexpr float SL_DEFAULT_ASPECT_RATIO = -1.0f;
  static constexpr float SL_DEFAULT_BORDER_RADIUS = 0.0f;

  static constexpr bool SL_DEFAULT_RELATIVE_LAYOUT_ONCE = true;

  static constexpr FlexDirectionType SL_DEFAULT_FLEX_DIRECTION =
      FlexDirectionType::kRow;
  static constexpr FlexWrapType SL_DEFAULT_FLEX_WRAP = FlexWrapType::kNowrap;
  static constexpr JustifyContentType SL_DEFAULT_JUSTIFY_CONTENT =
      JustifyContentType::kStretch;
  static constexpr FlexAlignType SL_DEFAULT_ALIGN_ITEMS =
      FlexAlignType::kStretch;
  static constexpr FlexAlignType SL_DEFAULT_ALIGN_SELF = FlexAlignType::kAuto;
  static constexpr AlignContentType SL_DEFAULT_ALIGN_CONTENT =
      AlignContentType::kStretch;
  static constexpr DisplayType SL_DEFAULT_DISPLAY = DisplayType::kAuto;
  static constexpr PositionType SL_DEFAULT_POSITION = PositionType::kRelative;
  static constexpr DirectionType SL_DEFAULT_DIRECTION = DirectionType::kNormal;
  static constexpr LinearOrientationType SL_DEFAULT_LINEAR_ORIENTATION =
      LinearOrientationType::kVertical;
  static constexpr LinearLayoutGravityType SL_DEFAULT_LINEAR_LAYOUT_GRAVITY =
      LinearLayoutGravityType::kNone;
  static constexpr LinearGravityType SL_DEFAULT_LINEAR_GRAVITY =
      LinearGravityType::kNone;
  static constexpr LinearCrossGravityType SL_DEFAULT_LINEAR_CROSS_GRAVITY =
      LinearCrossGravityType::kNone;
  static constexpr RelativeCenterType SL_DEFAULT_RELATIVE_CENTER =
      RelativeCenterType::kNone;
  static constexpr BorderStyleType SL_DEFAULT_BORDER_STYLE =
      BorderStyleType::kSolid;
  static constexpr BoxSizingType SL_DEFAULT_BOX_SIZING = BoxSizingType::kAuto;
  static constexpr JustifyType SL_DEFAULT_JUSTIFY_SELF = JustifyType::kAuto;
  static constexpr JustifyType SL_DEFAULT_JUSTIFY_ITEMS = JustifyType::kStretch;
  static constexpr GridAutoFlowType SL_DEFAULT_GRID_AUTO_FLOW =
      GridAutoFlowType::kRow;

  static const NLength& SL_DEFAULT_AUTO_LENGTH() {
    static base::NoDestructor<NLength> l{NLength::MakeAutoNLength()};
    return *l;
  }

  static const NLength& SL_DEFAULT_ZEROLENGTH() {
    static base::NoDestructor<NLength> l{NLength::MakeUnitNLength(0.0f)};
    return *l;
  }

  static const NLength SL_DEFAULT_WIDTH() { return SL_DEFAULT_AUTO_LENGTH(); }

  static const NLength SL_DEFAULT_HEIGHT() { return SL_DEFAULT_AUTO_LENGTH(); }

  static const NLength SL_DEFAULT_MIN_WIDTH() {
    static base::NoDestructor<NLength> l{
        NLength::MakeUnitNLength(kDefaultMinSize)};
    return *l;
  }

  static const NLength SL_DEFAULT_MAX_WIDTH() {
    static base::NoDestructor<NLength> l{
        NLength::MakeUnitNLength(kDefaultMaxSize)};
    return *l;
  }

  static const NLength SL_DEFAULT_MIN_HEIGHT() {
    static base::NoDestructor<NLength> l{
        NLength::MakeUnitNLength(kDefaultMinSize)};
    return *l;
  }

  static const NLength SL_DEFAULT_MAX_HEIGHT() {
    static base::NoDestructor<NLength> l{
        NLength::MakeUnitNLength(kDefaultMaxSize)};
    return *l;
  }
  static const NLength SL_DEFAULT_FLEX_BASIS() {
    return SL_DEFAULT_AUTO_LENGTH();
  }

  static const NLength SL_DEFAULT_FOUR_POSITION() {
    return SL_DEFAULT_AUTO_LENGTH();
  }

  static const NLength SL_DEFAULT_PADDING() { return SL_DEFAULT_ZEROLENGTH(); }

  static const NLength SL_DEFAULT_MARGIN() { return SL_DEFAULT_ZEROLENGTH(); }

  static constexpr float SL_DEFAULT_BORDER = 0.f;

  static const NLength SL_DEFAULT_RADIUS() { return SL_DEFAULT_ZEROLENGTH(); }

  static const NLength SL_DEFAULT_GRID_GAP() { return SL_DEFAULT_ZEROLENGTH(); }

  static std::vector<NLength> SL_DEFAULT_GRID_TRACK() {
    static base::NoDestructor<std::vector<NLength>> l{std::vector<NLength>()};
    return *l;
  }

  static constexpr BorderStyleType W3C_DEFAULT_BORDER_STYLE =
      BorderStyleType::kNone;
  static constexpr float W3C_DEFAULT_BORDER =
      static_cast<float>(BorderWidthType::kMedium);
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_DEFAULT_LAYOUT_STYLE_H_
