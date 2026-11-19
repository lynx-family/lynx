// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/custom_layout.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <optional>
#include <utility>

#include "base/include/auto_reset.h"
#include "base/include/compiler_specific.h"
#include "base/include/float_comparison.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/modifier_element.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace tasm {

namespace {

constexpr char kModifierPlacementX[] = "x";
constexpr char kModifierPlacementY[] = "y";

NO_INLINE float ReadLayoutNumber(const lepus::Value& value,
                                 const base::String& key, float scale) {
  const auto property = value.GetProperty(key);
  return property.IsNumber() ? property.Number() / scale
                             : std::numeric_limits<float>::quiet_NaN();
}

// Project placement order into the existing stacking machinery without changing
// the authored styles. GetElementStyle remains the source for restoration.
NO_INLINE void SetPlacementZIndex(Element& element, std::optional<int> order) {
  if (element.will_destroy()) return;
  auto* style = element.computed_css_style();
  auto authored = element.GetElementStyle(kPropertyIDZIndex);
  const CSSValue projected =
      order ? CSSValue(lepus::Value(*order), CSSValuePattern::NUMBER)
            : CSSValue();
  const auto& value = !order && authored ? *authored : projected;
  const bool changed =
      style->SetValue(kPropertyIDZIndex, value, !order && !authored);
  // The projected value must not become the declared value in the new styling
  // pipeline, which also uses this map when resolving subsequent patches.
  if (authored) {
    style->SetResolvedValue(kPropertyIDZIndex, *authored);
  } else {
    style->RemoveResolvedValue(kPropertyIDZIndex);
  }
  if (changed) {
    if (auto* container = element.element_container())
      container->UpdatePlacementZIndex();
  }
}

void RestoreZIndex(Element& element) {
  SetPlacementZIndex(element, std::nullopt);
}

}  // namespace

thread_local CustomLayout* CustomLayout::active_ = nullptr;

fml::RefPtr<CustomLayout> CustomLayout::Create(
    Element& owner, runtime::MTSRuntime& runtime,
    const lepus::Value& measure_callback, const lepus::Value& place_callback) {
  return fml::AdoptRef(
      new CustomLayout(owner, runtime, measure_callback, place_callback));
}

CustomLayout::CustomLayout(Element& owner, runtime::MTSRuntime& runtime,
                           const lepus::Value& measure_callback,
                           const lepus::Value& place_callback)
    : owner_(&owner), runtime_(runtime) {
  // JS callbacks stay weak and are traced by the handle's GC visitor.
  // CopyWeakValue retains reference-counted ownership for native closures.
  // Keep weak Values in place; moving them can promote them to strong roots.
  measure_callback_.CopyWeakValue(measure_callback);
  place_callback_.CopyWeakValue(place_callback);
}

CustomLayout::~CustomLayout() = default;

void CustomLayout::visitor(void* rt, void* func, uint64_t trace_tool) const {
  auto* runtime = static_cast<LEPUSRuntime*>(rt);
  auto* mark = reinterpret_cast<LEPUS_MarkFunc*>(func);
  // Native closures are retained by Value and do not belong to the JS heap.
  if (measure_callback_.IsJSValue())
    mark(runtime, WRAP_AS_JS_VALUE(measure_callback_.value()), trace_tool);
  if (place_callback_.IsJSValue())
    mark(runtime, WRAP_AS_JS_VALUE(place_callback_.value()), trace_tool);
}

void CustomLayout::ResetPlacementOrder() {
  for (auto& child : children_) RestoreZIndex(*child.element);
  RestoreZIndex(*owner_);
}

void CustomLayout::Attach() {
  owner_->SetCustomLayoutFunctions({
      nullptr,
      [](void* context) { static_cast<CustomLayout*>(context)->Alignment(); },
      nullptr,
      this,
      [](void* context, const starlight::ComposeConstraints& constraints,
         bool final_measure) {
        const auto size = static_cast<CustomLayout*>(context)->Measure(
            constraints, final_measure);
        return FloatSize(size.width_, size.height_, size.baseline_);
      },
  });
}

void CustomLayout::Detach() {
  // A callback can release its handle while keep_alive retains this adapter.
  // Prevent that in-flight pass from committing placement after cleanup.
  valid_ = false;
  const bool is_modifier = owner_->is_modifier();
  if (is_modifier) static_cast<ModifierElement&>(*owner_).ClearLayoutBinding();
  if (!owner_->will_destroy()) {
    owner_->SetCustomLayoutFunctions({});
    // A measure can run before the queued Element flush. Clear the live hooks
    // before releasing the adapter.
    if (auto* node = owner_->slnode()) {
      node->SetContext(nullptr);
      node->SetSLMeasureFunc(nullptr);
      node->SetSLComposeMeasureFunc(nullptr);
      node->SetSLAlignmentFunc(nullptr);
      node->SetCanReuseLayoutWithSameSizeAsGivenConstraintFunc(nullptr);
      node->MarkDirty();
    }
  }
  if (!is_modifier) ResetPlacementOrder();
}

void CustomLayout::Invalidate() {
  valid_ = false;
  // RequestLayout can defer dirty propagation until the next flush. Invalidate
  // the live measurement immediately, including from inside a policy callback.
  if (owner_->slnode()) owner_->slnode()->MarkDirty();
  owner_->RequestLayout();
}

bool CustomLayout::IsChild(Element& child) const {
  return !child.will_destroy() && child.slnode() && owner_->slnode() &&
         child.slnode()->ParentLayoutObject() == owner_->slnode();
}

LayoutResult CustomLayout::Measure(
    const starlight::ComposeConstraints& constraints, bool final_measure) {
  fml::RefPtr<CustomLayout> keep_alive(this);
  if (IsExecuting()) return {};
  base::AutoReset<Phase> phase(&phase_, Phase::kMeasure);
  base::AutoReset<CustomLayout*> active(&active_, this);
  final_measure_ = final_measure;
  const bool is_modifier = owner_->is_modifier();
  // Preserve projections for children measured again to avoid invalidating
  // cached measurements while restoring and reapplying stacking contexts.
  auto previous_children =
      is_modifier ? base::Vector<Child>() : std::move(children_);
  children_.clear();
  valid_ = true;
  auto restore_unused_projection = [&] {
    if (is_modifier) return;
    for (auto& previous : previous_children) {
      if (!valid_ ||
          std::none_of(children_.begin(), children_.end(), [&](const auto& c) {
            return c.element == previous.element;
          })) {
        RestoreZIndex(*previous.element);
      }
    }
    if (!valid_) ResetPlacementOrder();
  };
  float scale = owner_->computed_css_style()
                    ->GetLayoutComputedStyle()
                    ->PhysicalPixelsPerLayoutUnit();
  auto maximum = [scale](float bound) {
    return std::isinf(bound) ? -1.f : bound * scale;
  };
  auto value = runtime_.CallClosure(
      measure_callback_, lepus::Value(constraints.min_width * scale),
      lepus::Value(maximum(constraints.max_width)),
      lepus::Value(constraints.min_height * scale),
      lepus::Value(maximum(constraints.max_height)));
  const auto width = ReadLayoutNumber(value, BASE_STATIC_STRING(kWidth), scale);
  const auto height =
      ReadLayoutNumber(value, BASE_STATIC_STRING(kHeight), scale);
  LayoutResult result(width, height);
  valid_ = valid_ && std::isfinite(result.width_) && result.width_ >= 0 &&
           std::isfinite(result.height_) && result.height_ >= 0;
  if (valid_ && is_modifier) {
    const auto x =
        ReadLayoutNumber(value, BASE_STATIC_STRING(kModifierPlacementX), scale);
    const auto y =
        ReadLayoutNumber(value, BASE_STATIC_STRING(kModifierPlacementY), scale);
    valid_ =
        valid_ && children_.size() == 1 && std::isfinite(x) && std::isfinite(y);
    if (valid_) {
      auto& child = children_.front();
      child.x = x;
      child.y = y;
      valid_ = IsChild(*child.element);
      if (valid_) {
        result.width_ = constraints.ConstrainWidth(result.width_);
        result.height_ = constraints.ConstrainHeight(result.height_);
        result.baseline_ =
            child.element->slnode()->GetOffsetFromTopMarginEdgeToBaseline() +
            child.y;
      }
    }
  }
  restore_unused_projection();
  return valid_ ? result : LayoutResult();
}

CustomLayout* CustomLayout::ActiveModifierMeasure(
    runtime::MTSRuntime& runtime) {
  auto* layout = active_;
  return layout && layout->phase_ == Phase::kMeasure && layout->valid_ &&
                 layout->UsesRuntime(runtime) && layout->owner_->is_modifier()
             ? layout
             : nullptr;
}

Element* CustomLayout::ModifierChild() const {
  return owner_->GetChildCount() == 1 ? owner_->GetChildAt(0) : nullptr;
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
  const starlight::ComposeConstraints constraints{
      min_width,
      max_width == -1 ? starlight::ComposeConstraints::kInfinity : max_width,
      min_height,
      max_height == -1 ? starlight::ComposeConstraints::kInfinity : max_height};
  auto size = child.slnode()->UpdateMeasureWithComposeConstraints(
      constraints, final_measure_);
  // Starlight retains subpixel-equal dimensions in its geometry setters.
  // Density conversion and padding arithmetic can therefore leave an old
  // dimension one ULP outside an exact range. Reject real violations, then
  // normalize the reported size to the authoritative Compose bounds.
  auto outside = [](float size, float min, float max) {
    return base::FloatsLargerPrecise(min, size) ||
           (max != -1 && base::FloatsLargerPrecise(size, max));
  };
  if (!std::isfinite(size.width_) || !std::isfinite(size.height_) ||
      outside(size.width_, min_width, max_width) ||
      outside(size.height_, min_height, max_height)) {
    valid_ = false;
    return false;
  }
  children_.push_back(Child{fml::RefPtr<Element>(&child)});
  result =
      LayoutResult(constraints.ConstrainWidth(size.width_),
                   constraints.ConstrainHeight(size.height_), size.baseline_);
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
  fml::RefPtr<CustomLayout> keep_alive(this);
  if (IsExecuting() || !valid_) return;
  base::AutoReset<Phase> phase(&phase_, Phase::kPlace);
  base::AutoReset<CustomLayout*> active(&active_, this);
  if (owner_->is_modifier()) {
    auto& child = children_.front();
    if (!IsChild(*child.element)) return;
    child.element->slnode()->AlignmentByPlatform(child.y, child.x);
    if (valid_ && final_measure_) {
      const auto& frame = static_cast<const ModifierElement&>(*owner_);
      runtime_.CallClosure(place_callback_, lepus::Value(frame.impl_id()),
                           lepus::Value(frame.layout_epoch()));
    }
    return;
  }
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
  base::Vector<Child*> placed;
  for (auto& child : children_) {
    if (child.placed) placed.push_back(&child);
  }
  // Sort only pointers through the shared C library implementation, avoiding
  // a separate sorting template for this policy's private record type.
  if (placed.size() > 1) {
    std::qsort(placed.data(), placed.size(), sizeof(Child*),
               [](const void* left, const void* right) {
                 const auto* a = *static_cast<Child* const*>(left);
                 const auto* b = *static_cast<Child* const*>(right);
                 if (a->z != b->z) return a->z < b->z ? -1 : 1;
                 return (a->order > b->order) - (a->order < b->order);
               });
  }
  // Keep nested z children inside this layout's ordinary stacking context.
  if (!owner_->computed_css_style()->HasZIndex())
    SetPlacementZIndex(*owner_, 0);
  for (size_t i = 0; i < placed.size(); ++i) {
    auto& child = *placed[i];
    // Map floating-point z and placement order to the existing integer
    // stacking order. The policy owns the order of these siblings.
    SetPlacementZIndex(*child.element, static_cast<int>(i) + 1);
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
