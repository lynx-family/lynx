// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_DYNAMIC_DIRECTION_STYLES_MANAGER_H_
#define CORE_RENDERER_CSS_DYNAMIC_DIRECTION_STYLES_MANAGER_H_

#include <map>
#include <utility>

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {
using CSSStyleValue = std::pair<CSSPropertyID, CSSValue>;
using IsLogic = bool;
bool IsDirectionAwareStyle(CSSPropertyID css_id);

bool IsLogicalDirectionStyle(CSSPropertyID css_id);

inline bool IsRTL(starlight::DirectionType direction) {
  return direction == starlight::DirectionType::kRtl;
}

inline bool IsLynxRTL(starlight::DirectionType direction) {
  return direction == starlight::DirectionType::kLynxRtl;
}

inline bool IsAnyRTL(starlight::DirectionType direction) {
  return IsRTL(direction) || IsLynxRTL(direction);
}

CSSPropertyID ResolveDirectionAwareProperty(CSSPropertyID css_id,
                                            starlight::DirectionType direction);
CSSStyleValue ResolveTextAlign(CSSPropertyID css_id,
                               const tasm::CSSValue& value,
                               starlight::DirectionType direction);

std::pair<CSSPropertyID, IsLogic> ResolveLogicStyleID(CSSPropertyID css_id);
CSSPropertyID ResolveDirectionRelatedStyleID(CSSPropertyID trans_id,
                                             starlight::DirectionType direction,
                                             bool is_logic_style);
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_DYNAMIC_DIRECTION_STYLES_MANAGER_H_
