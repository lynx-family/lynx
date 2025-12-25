// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_DYNAMIC_CSS_STYLES_MANAGER_H_
#define CORE_RENDERER_CSS_DYNAMIC_CSS_STYLES_MANAGER_H_

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_set>
#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/dynamic_css_configs.h"
#include "core/renderer/css/dynamic_direction_styles_manager.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/layout_configs.h"

namespace lynx {
namespace tasm {

struct PseudoPlaceHolderStyles {
  std::optional<CSSValue> font_size_;
  std::optional<CSSValue> color_;
  std::optional<CSSValue> font_weight_;
  std::optional<CSSValue> font_family_;
};

class Element;
class LayoutNode;

struct PropertiesResolvingStatus {
  // per page status
  struct PageStatus {
    float root_font_size_;
    float font_scale_ = Config::DefaultFontScale();
    starlight::LayoutUnit viewport_width_;
    starlight::LayoutUnit viewport_height_;
    float screen_width_ = 0.f;
  };

  PageStatus page_status_;

  // per element status
  float computed_font_size_;
  starlight::DirectionType direction_type_ =
      starlight::DefaultLayoutStyle::SL_DEFAULT_DIRECTION;

  void ApplyPageStatus(const PropertiesResolvingStatus& status) {
    page_status_ = status.page_status_;
  }
};

class DynamicCSSStylesManager {
 private:
  enum StyleDynamicType : uint32_t {
    kEmType = 0,
    kRemType = 1,
    kScreenMetricsType = 2,
    kDirectionStyleType = 3,
    kFontScaleType = 4,
    kViewportType = 5,
    kDynamicTypeCount = 6
  };

 public:
  DynamicCSSStylesManager(){};

  enum StyleUpdateFlag : uint32_t {
    kUpdateEm = 1 << kEmType,
    kUpdateRem = 1 << kRemType,
    kUpdateScreenMetrics = 1 << kScreenMetricsType,
    kUpdateDirectionStyle = 1 << kDirectionStyleType,
    kUpdateFontScale = 1 << kFontScaleType,
    kUpdateViewport = 1 << kViewportType,
  };

  static constexpr uint32_t kAllStyleUpdate =
      kUpdateEm | kUpdateRem | kUpdateScreenMetrics | kUpdateDirectionStyle |
      kUpdateFontScale | kUpdateViewport;

  static constexpr uint32_t kNoUpdate = 0;

  using StyleUpdateFlags = uint32_t;

  static const std::unordered_set<CSSPropertyID>& GetInheritableProps();

  static CSSPropertyID ResolveDirectionAwarePropertyID(
      CSSPropertyID id, starlight::DirectionType direction);

  static void UpdateDirectionAwareDefaultStyles(
      Element* element, starlight::DirectionType direction,
      const CSSValue& text_align_value);

  static StyleUpdateFlags GetValueFlags(CSSPropertyID id, const CSSValue& value,
                                        bool unify_vw_vh_behavior);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_DYNAMIC_CSS_STYLES_MANAGER_H_
