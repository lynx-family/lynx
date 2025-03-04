// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/property_resolving_utils.h"

#include <algorithm>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"
#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace starlight {

namespace property_utils {
namespace {

void ApplyAspectRatio(const LayoutComputedStyle& css,
                      DimensionValue<LayoutUnit>& size) {
  const float aspect_ratio = css.GetAspectRatio();
  if (aspect_ratio == -1.0f) {
    return;
  }

  if (size[kHorizontal].IsDefinite()) {
    if (size[kVertical].IsIndefinite()) {
      size[kVertical] = size[kHorizontal].ToFloat() / aspect_ratio;
    }
  } else {
    if (size[kVertical].IsDefinite()) {
      size[kHorizontal] = size[kVertical].ToFloat() * aspect_ratio;
    }
  }
}

void ResolveFitContent(const LayoutObject& item,
                       const Constraints& container_constraint,
                       Dimension direction,
                       OneSideConstraint& one_side_constraint) {
  const NLength& length =
      logic_direction_utils::GetCSSDimensionSize(item.GetCSSStyle(), direction);
  if (!length.IsFitContent()) {
    return;
  }

  if (!length.NumericLength().HasValue()) {
    if (container_constraint[direction].Mode() != SLMeasureModeIndefinite) {
      one_side_constraint =
          OneSideConstraint::AtMost(container_constraint[direction].Size());
    } else {
      one_side_constraint = OneSideConstraint::Indefinite();
    }
  } else {
    LayoutUnit fit_value = NLengthToLayoutUnit(
        length, container_constraint[direction].ToPercentBase());
    one_side_constraint =
        OneSideConstraint::AtMost(fit_value.ClampIndefiniteToZero().ToFloat());
  }
}
}  // namespace

// static
void HandleBoxSizing(const LayoutComputedStyle& style, const BoxInfo& box_info,
                     DimensionValue<LayoutUnit>& size,
                     const LayoutConfigs& layout_config) {
  if (style.IsBorderBox(layout_config)) {
    return;
  }
  const auto& padding = box_info.padding_;
  size[kHorizontal] = size[kHorizontal] + padding[kLeft] + padding[kRight] +
                      style.GetBorderFinalLeftWidth() +
                      style.GetBorderFinalRightWidth();
  size[kVertical] = size[kVertical] + padding[kTop] + padding[kBottom] +
                    style.GetBorderFinalTopWidth() +
                    style.GetBorderFinalBottomWidth();
}

DimensionValue<LayoutUnit> ComputePreferredSize(
    const LayoutObject& item, const Constraints& container_constraint) {
  DimensionValue<LayoutUnit> result;

  const auto& css = *item.GetCSSStyle();
  result[kHorizontal] = NLengthToLayoutUnit(
      css.GetWidth(), container_constraint[kHorizontal].ToPercentBase());
  result[kVertical] = NLengthToLayoutUnit(
      css.GetHeight(), container_constraint[kVertical].ToPercentBase());

  if (!css.GetWidth().IsMaxContent() && !css.GetHeight().IsMaxContent()) {
    ApplyAspectRatio(css, result);
  }
  HandleBoxSizing(css, *item.GetBoxInfo(), result, item.GetLayoutConfigs());
  return result;
}

void ApplyAspectRatio(const LayoutObject* layout_object, Constraints& size) {
  const auto& css = *layout_object->GetCSSStyle();
  const float aspect_ratio = css.GetAspectRatio();
  if (aspect_ratio == -1.0f) {
    return;
  }
  if ((size[kHorizontal].Mode() != SLMeasureModeDefinite &&
       size[kVertical].Mode() != SLMeasureModeDefinite) ||
      (size[kHorizontal].Mode() == SLMeasureModeDefinite &&
       size[kVertical].Mode() == SLMeasureModeDefinite)) {
    return;
  }

  DimensionValue<LayoutUnit> result{size[kHorizontal].ToPercentBase(),
                                    size[kVertical].ToPercentBase()};
  if (css.IsBorderBox(layout_object->GetLayoutConfigs())) {
    ApplyAspectRatio(css, result);
    if (result[kHorizontal].IsDefinite()) {
      size[kHorizontal].ApplySize(result[kHorizontal]);
    }
    if (result[kVertical].IsDefinite()) {
      size[kVertical].ApplySize(result[kVertical]);
    }
  } else {
    float padding_border_width =
        logic_direction_utils::GetPaddingAndBorderDimensionSize(layout_object,
                                                                kHorizontal);
    float padding_border_height =
        logic_direction_utils::GetPaddingAndBorderDimensionSize(layout_object,
                                                                kVertical);
    if (result[kHorizontal].IsDefinite()) {
      result[kHorizontal] =
          result[kHorizontal].ToFloat() - padding_border_width;
    }
    if (result[kVertical].IsDefinite()) {
      result[kVertical] = result[kVertical].ToFloat() - padding_border_height;
    }

    ApplyAspectRatio(css, result);

    if (result[kHorizontal].IsDefinite()) {
      result[kHorizontal] =
          result[kHorizontal].ToFloat() + padding_border_width;
      size[kHorizontal].ApplySize(result[kHorizontal]);
    }
    if (result[kVertical].IsDefinite()) {
      result[kVertical] = result[kVertical].ToFloat() + padding_border_height;
      size[kVertical].ApplySize(result[kVertical]);
    }
  }
}

Constraints GenerateDefaultConstraints(
    const LayoutObject& item, const Constraints& container_constraint) {
  Constraints result;

  const auto preferred_size =
      property_utils::ComputePreferredSize(item, container_constraint);

  if (preferred_size[kHorizontal].IsDefinite()) {
    result[kHorizontal] =
        OneSideConstraint::Definite(preferred_size[kHorizontal].ToFloat());
  } else if (container_constraint[kHorizontal].Mode() !=
             SLMeasureModeIndefinite) {
    result[kHorizontal] = OneSideConstraint::AtMost(StripMargins(
        container_constraint[kHorizontal].Size(), item, kHorizontal));
  }

  if (preferred_size[kVertical].IsDefinite()) {
    result[kVertical] =
        OneSideConstraint::Definite(preferred_size[kVertical].ToFloat());
  } else if (container_constraint[kVertical].Mode() !=
             SLMeasureModeIndefinite) {
    result[kVertical] = OneSideConstraint::AtMost(
        StripMargins(container_constraint[kVertical].Size(), item, kVertical));
  }

  if (item.GetCSSStyle()->GetWidth().IsFitContent()) {
    ResolveFitContent(item, container_constraint, kHorizontal,
                      result[kHorizontal]);
  }

  if (item.GetCSSStyle()->GetHeight().IsFitContent()) {
    ResolveFitContent(item, container_constraint, kVertical, result[kVertical]);
  }

  if (item.GetCSSStyle()->GetWidth().IsMaxContent()) {
    result[kHorizontal] = OneSideConstraint::Indefinite();
  }
  if (item.GetCSSStyle()->GetHeight().IsMaxContent()) {
    result[kVertical] = OneSideConstraint::Indefinite();
  }
  return result;
}

float StripMargins(float value, const LayoutObject& obj, Dimension dimension) {
  if (dimension == kHorizontal) {
    return value - obj.GetLayoutMarginLeft() - obj.GetLayoutMarginRight();
  } else {
    return value - obj.GetLayoutMarginTop() - obj.GetLayoutMarginBottom();
  }
}

void ApplyMinMaxToConstraints(Constraints& constraints,
                              const LayoutObject& item) {
  if (constraints[kHorizontal].Mode() != SLMeasureModeIndefinite) {
    constraints[kHorizontal] =
        OneSideConstraint(item.ClampExactWidth(constraints[kHorizontal].Size()),
                          constraints[kHorizontal].Mode());
  } else if (item.GetBoxInfo()->max_size_[kHorizontal] !=
             DefaultLayoutStyle::kDefaultMaxSize) {
    constraints[kHorizontal] =
        OneSideConstraint::AtMost(item.GetBoxInfo()->max_size_[kHorizontal]);
  }
  if (constraints[kVertical].Mode() != SLMeasureModeIndefinite) {
    constraints[kVertical] =
        OneSideConstraint(item.ClampExactHeight(constraints[kVertical].Size()),
                          constraints[kVertical].Mode());
  } else if (item.GetBoxInfo()->max_size_[kVertical] !=
             DefaultLayoutStyle::kDefaultMaxSize) {
    constraints[kVertical] =
        OneSideConstraint::AtMost(item.GetBoxInfo()->max_size_[kVertical]);
  }
}

float ApplyMinMaxToSpecificSize(float size, const LayoutObject* item,
                                Dimension dimension) {
  const float border_and_padding_size =
      logic_direction_utils::GetPaddingAndBorderDimensionSize(item, dimension);
  BoxInfo* box_info = item->GetBoxInfo();
  const float max_size =
      box_info->max_size_[dimension] - border_and_padding_size;
  const float min_size =
      box_info->min_size_[dimension] - border_and_padding_size;

  size = std::min(size, max_size);
  size = std::max(size, min_size);
  size = std::max(size, 0.0f);
  return size;
}

}  // namespace property_utils
}  // namespace starlight
}  // namespace lynx
