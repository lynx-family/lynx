// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/custom_layout.h"

#include <algorithm>
#include <cmath>

#include "base/include/auto_reset.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace tasm {

namespace {

// Project placement order into the existing stacking machinery without changing
// the authored styles. GetElementStyle remains the source for restoration.
void SetPlacementZIndex(Element& element,
                        const std::optional<CSSValue>& value) {
  if (element.will_destroy()) return;
  auto* style = element.computed_css_style();
  auto authored = element.GetElementStyle(kPropertyIDZIndex);
  bool changed = style->SetValue(kPropertyIDZIndex, value.value_or(CSSValue()),
                                 !value.has_value());
  // The projected value must not become the declared value in the new styling
  // pipeline, which also uses this map when resolving subsequent patches.
  if (authored) {
    style->SetResolvedValue(kPropertyIDZIndex, *authored);
  } else {
    style->RemoveResolvedValue(kPropertyIDZIndex);
  }
  if (changed) {
    if (auto* container = element.element_container())
      container->StyleChanged();
  }
}

void RestoreZIndex(Element& element) {
  SetPlacementZIndex(element, element.GetElementStyle(kPropertyIDZIndex));
}

}  // namespace

thread_local CustomLayout* CustomLayout::active_ = nullptr;

CustomLayout::CustomLayout(Element& owner, runtime::MTSRuntime& runtime,
                           const lepus::Value& measure_callback,
                           const lepus::Value& place_callback)
    : owner_(&owner), runtime_(runtime) {
  // Match ListElement callbacks: weak values plus the handle's GC visitor.
  // Keep weak Values in place; moving them can promote them to strong roots.
  measure_callback_.CopyWeakValue(measure_callback);
  place_callback_.CopyWeakValue(place_callback);
}

void CustomLayout::visitor(void* rt, void* func, uint64_t trace_tool) const {
  auto* runtime = static_cast<LEPUSRuntime*>(rt);
  auto* mark = reinterpret_cast<LEPUS_MarkFunc*>(func);
  mark(runtime, WRAP_AS_JS_VALUE(measure_callback_.value()), trace_tool);
  mark(runtime, WRAP_AS_JS_VALUE(place_callback_.value()), trace_tool);
}

void CustomLayout::ResetPlacementOrder() {
  for (auto& child : children_) RestoreZIndex(*child.element);
  RestoreZIndex(*owner_);
}

void CustomLayout::Detach() {
  // A callback can release its handle while keep_alive retains this adapter.
  // Prevent that in-flight pass from committing placement after cleanup.
  valid_ = false;
  ResetPlacementOrder();
}

void CustomLayout::Invalidate() {
  valid_ = false;
  owner_->RequestLayout();
}

bool CustomLayout::IsChild(Element& child) const {
  return !child.will_destroy() && child.slnode() && owner_->slnode() &&
         child.slnode()->ParentLayoutObject() == owner_->slnode();
}

LayoutResult CustomLayout::Measure(float width, int32_t width_mode,
                                   float height, int32_t height_mode,
                                   bool final_measure) {
  auto keep_alive = shared_from_this();
  if (IsExecuting()) return {};
  base::AutoReset<Phase> phase(&phase_, Phase::kMeasure);
  base::AutoReset<CustomLayout*> active(&active_, this);
  final_measure_ = final_measure;
  ResetPlacementOrder();
  children_.clear();
  valid_ = true;
  float scale = owner_->computed_css_style()
                    ->GetLayoutComputedStyle()
                    ->PhysicalPixelsPerLayoutUnit();
  auto value = runtime_.CallClosure(
      measure_callback_, lepus::Value(width * scale), lepus::Value(width_mode),
      lepus::Value(height * scale), lepus::Value(height_mode));
  auto w = value.GetProperty(BASE_STATIC_STRING(kWidth));
  auto h = value.GetProperty(BASE_STATIC_STRING(kHeight));
  if (!w.IsNumber() || !h.IsNumber()) {
    valid_ = false;
    return {};
  }
  LayoutResult result(w.Number() / scale, h.Number() / scale);
  valid_ = valid_ && std::isfinite(result.width_) && result.width_ >= 0 &&
           std::isfinite(result.height_) && result.height_ >= 0;
  return valid_ ? result : LayoutResult();
}

bool CustomLayout::MeasureChild(Element& child, float min_width,
                                float max_width, float min_height,
                                float max_height, LayoutResult& result) {
  if (active_ != this || phase_ != Phase::kMeasure || !IsChild(child) ||
      !std::isfinite(min_width) || !std::isfinite(min_height) ||
      !std::isfinite(max_width) || !std::isfinite(max_height) ||
      min_width < 0 || min_height < 0 ||
      (max_width != -1 && max_width < min_width) ||
      (max_height != -1 && max_height < min_height) ||
      std::any_of(children_.begin(), children_.end(), [&child](const auto& c) {
        return c.element.get() == &child;
      })) {
    valid_ = false;
    return false;
  }
  using starlight::OneSideConstraint;
  auto constraint = [](float min, float max) {
    if (min == max) return OneSideConstraint::Definite(max);
    return max == -1 ? OneSideConstraint::Indefinite()
                     : OneSideConstraint::AtMost(max);
  };
  starlight::Constraints constraints;
  constraints[starlight::kHorizontal] = constraint(min_width, max_width);
  constraints[starlight::kVertical] = constraint(min_height, max_height);
  auto size =
      child.slnode()->UpdateMeasureByPlatform(constraints, final_measure_);
  // Platform measurement treats constraints as a CSS containing block.
  // CSS sizes are content hints; the policy owns the final size bounds.
  auto outside = [](float size, float min, float max) {
    return size < min || (max != -1 && size > max);
  };
  if (outside(size.width_, min_width, max_width) ||
      outside(size.height_, min_height, max_height)) {
    auto* node = child.slnode();
    constraints = starlight::property_utils::GenerateDefaultConstraints(
        *node, constraints);
    float margin_width =
        node->GetLayoutMarginLeft() + node->GetLayoutMarginRight();
    float margin_height =
        node->GetLayoutMarginTop() + node->GetLayoutMarginBottom();
    auto exact = [](float size, float min, float max, float margin) {
      return OneSideConstraint::Definite(std::max(
          0.f, std::max(min, max == -1 ? size : std::min(size, max)) - margin));
    };
    if (outside(size.width_, min_width, max_width))
      constraints[starlight::kHorizontal] =
          exact(size.width_, min_width, max_width, margin_width);
    if (outside(size.height_, min_height, max_height))
      constraints[starlight::kVertical] =
          exact(size.height_, min_height, max_height, margin_height);
    // Reusing CSS min/max here would apply the same conflicting clamp again.
    // Override only resolved bounds for this measurement, not authored styles
    // or the child's content measurement implementation.
    auto* box = node->GetBoxInfo();
    base::AutoReset<starlight::DimensionValue<float>> min_size(
        &box->min_size_, starlight::DimensionValue<float>{0.f, 0.f});
    base::AutoReset<starlight::DimensionValue<float>> max_size(
        &box->max_size_, starlight::DimensionValue<float>{
                             starlight::DefaultLayoutStyle::kDefaultMaxSize,
                             starlight::DefaultLayoutStyle::kDefaultMaxSize});
    size = node->UpdateMeasure(constraints, final_measure_);
    size.width_ += margin_width;
    size.height_ += margin_height;
  }
  if (!std::isfinite(size.width_) || !std::isfinite(size.height_) ||
      outside(size.width_, min_width, max_width) ||
      outside(size.height_, min_height, max_height)) {
    valid_ = false;
    return false;
  }
  children_.push_back({fml::RefPtr<Element>(&child)});
  result = LayoutResult(size.width_, size.height_, size.baseline_);
  return true;
}

bool CustomLayout::PlaceChild(Element& child, float x, float y, float z) {
  if (active_ != this || phase_ != Phase::kPlace || !IsChild(child) ||
      !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
    valid_ = false;
    return false;
  }
  for (auto& record : children_) {
    if (record.element.get() == &child && !record.placed) {
      record.placed = true;
      record.x = x;
      record.y = y;
      record.z = z;
      record.order = placement_order_++;
      return true;
    }
  }
  valid_ = false;
  return false;
}

void CustomLayout::Alignment() {
  auto keep_alive = shared_from_this();
  if (IsExecuting() || !valid_) return;
  base::AutoReset<Phase> phase(&phase_, Phase::kPlace);
  base::AutoReset<CustomLayout*> active(&active_, this);
  placement_order_ = 0;
  for (auto& child : children_) child.placed = false;
  auto result = runtime_.CallClosure(place_callback_);
  if (!result.IsBool() || !result.Bool() || !valid_) return;
  for (auto& child : children_) {
    if (!IsChild(*child.element)) {
      valid_ = false;
      return;
    }
  }
  std::vector<Child*> placed;
  for (auto& child : children_) {
    if (child.placed) placed.push_back(&child);
  }
  std::sort(placed.begin(), placed.end(), [](const auto* a, const auto* b) {
    return a->z == b->z ? a->order < b->order : a->z < b->z;
  });
  // Keep nested z children inside this layout's ordinary stacking context.
  if (!owner_->computed_css_style()->HasZIndex())
    SetPlacementZIndex(*owner_,
                       CSSValue(lepus::Value(0), CSSValuePattern::NUMBER));
  for (size_t i = 0; i < placed.size(); ++i) {
    auto& child = *placed[i];
    // Map floating-point z and placement order to the existing integer
    // stacking order. The policy owns the order of these siblings.
    SetPlacementZIndex(*child.element,
                       CSSValue(lepus::Value(static_cast<int>(i) + 1),
                                CSSValuePattern::NUMBER));
    child.element->slnode()->AlignmentByPlatform(child.y, child.x);
    // A nested policy may release this policy's handle during alignment.
    if (!valid_) return;
  }
  // Placement runs after the pipeline's style-time stacking sort.
  if (auto* container = owner_->element_container())
    container->UpdateZIndexList();
  for (auto* node = owner_->slnode()->FirstChild(); node; node = node->Next()) {
    auto* child = static_cast<starlight::LayoutObject*>(node);
    if (std::none_of(placed.begin(), placed.end(), [child](const auto* c) {
          return c->element->slnode() == child;
        })) {
      child->LayoutDisplayNone();
    }
  }
}

}  // namespace tasm
}  // namespace lynx
