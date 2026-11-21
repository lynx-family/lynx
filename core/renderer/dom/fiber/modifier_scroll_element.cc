// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/modifier_scroll_element.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "base/include/value/table.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"

namespace lynx {
namespace tasm {

ModifierScrollElement::ModifierScrollElement(ElementManager* manager)
    : ScrollElement(manager, BASE_STATIC_STRING(kElementScrollViewTag)) {
  MarkAsComposeLayout();
  ResetModifierStyles();
  InstallLayoutFunctions();
}

ModifierScrollElement::ModifierScrollElement(
    const ModifierScrollElement& element, bool clone_resolved_props)
    : ScrollElement(element, clone_resolved_props),
      binding_id_(element.binding_id_),
      vertical_(element.vertical_),
      reverse_(element.reverse_),
      rtl_(element.rtl_) {
  scroll_state_.Initialize(element.scroll_state_.offset());
  InstallLayoutFunctions();
}

fml::RefPtr<Element> ModifierScrollElement::CloneElement(
    bool clone_resolved_props) const {
  return fml::AdoptRef<Element>(
      new ModifierScrollElement(*this, clone_resolved_props));
}

void ModifierScrollElement::ResetModifierStyles() {
  SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
  SetStyle(kPropertyIDBoxSizing, lepus::Value("border-box"));
  // Send the viewport clip to native hosts even when hidden is the CSS default.
  SetStyle(kPropertyIDOverflow, lepus::Value("hidden"));
}

void ModifierScrollElement::InstallLayoutFunctions() {
  // The scroll boundary keeps the viewport constraints while measuring its
  // child with an unbounded maximum on the scroll axis.
  SetCustomLayoutFunctions({
      nullptr,
      [](void* context) {
        static_cast<ModifierScrollElement*>(context)->Align();
      },
      nullptr,
      this,
      [](void* context, const starlight::ComposeConstraints& constraints,
         bool final_measure) {
        return static_cast<ModifierScrollElement*>(context)->Measure(
            constraints, final_measure);
      },
  });
}

void ModifierScrollElement::Configure(int binding_id, bool vertical,
                                      bool reverse, bool rtl,
                                      float initial_offset_px) {
  if (binding_id_ == 0) {
    scroll_state_.Initialize(
        initial_offset_px /
        element_manager()->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit());
  }
  if (vertical_ != vertical || reverse_ != reverse || rtl_ != rtl) {
    scroll_state_.Invalidate();
    MarkLayoutDirtyLite();
  }
  binding_id_ = binding_id;
  vertical_ = vertical;
  reverse_ = reverse;
  rtl_ = rtl;
  SetAttribute(BASE_STATIC_STRING(kScrollOrientation),
               lepus::Value(vertical ? kVertical : kHorizontal));
  SetStyle(kPropertyIDDirection, lepus::Value(rtl ? "rtl" : "ltr"));
}

FloatSize ModifierScrollElement::Measure(
    const starlight::ComposeConstraints& constraints, bool final_measure) {
  if (final_measure) {
    const bool bounded = std::isfinite(vertical_ ? constraints.max_height
                                                 : constraints.max_width);
    if (bounded_viewport_ != bounded) scroll_state_.Invalidate();
    bounded_viewport_ = bounded;
  }
  auto child_constraints = constraints;
  // Preserve the minimum constraints, as Foundation's ScrollNode does.
  if (vertical_) {
    child_constraints.max_height = starlight::ComposeConstraints::kInfinity;
  } else {
    child_constraints.max_width = starlight::ComposeConstraints::kInfinity;
  }
  auto* child = GetChildAt(0);
  const auto size = child && child->slnode()
                        ? child->slnode()->UpdateMeasureWithComposeConstraints(
                              child_constraints, final_measure)
                        : FloatSize(0, 0);
  return FloatSize(constraints.ConstrainWidth(size.width_),
                   constraints.ConstrainHeight(size.height_));
}

void ModifierScrollElement::Align() {
  if (auto* child = GetChildAt(0); child && child->slnode()) {
    using starlight::BoundType;
    const auto& content = child->slnode()->GetLayoutResult().size_;
    const float x =
        rtl_ ? std::max(0.f, slnode()->GetContentBoundWidth() - content.width_)
             : 0.f;
    child->slnode()->SetBoundLeftFrom(slnode(), x, BoundType::kBorder,
                                      BoundType::kContent);
    child->slnode()->SetBoundTopFrom(slnode(), 0, BoundType::kBorder,
                                     BoundType::kContent);
    child->slnode()->UpdateAlignment();
  }
}

void ModifierScrollElement::OnSubtreeLayoutUpdated(PipelineOptions*) {
  const auto& border = borders();
  const float border_width =
      border[starlight::kLeft] + border[starlight::kRight];
  const float border_height =
      border[starlight::kTop] + border[starlight::kBottom];
  const float viewport =
      vertical_ ? height() - border_height : width() - border_width;
  auto* child = GetChildAt(0);
  const float content_width = child ? child->width() : 0;
  const float content_height = child ? child->height() : 0;
  const bool state_changed = scroll_state_.UpdateLayout(
      std::max(0.f, viewport), vertical_ ? content_height : content_width);
  // Native hosts scroll against their outer bounds. Include the borders so
  // their scroll range matches the Engine's clipped viewport.
  const float projected_width = std::max(width(), content_width + border_width);
  const float projected_height =
      std::max(height(), content_height + border_height);
  if (state_changed || projected_content_width_ != projected_width ||
      projected_content_height_ != projected_height) {
    projected_content_width_ = projected_width;
    projected_content_height_ = projected_height;
    Project();
  }
  if (state_changed) {
    // Publish measured bounds and any layout-clamped offset to ScrollState.
    element_manager()->SendNativeCustomEvent("modifierscroll", impl_id(),
                                             StateValue(), "detail");
  }
}

void ModifierScrollElement::Project() {
  // A host retains its last projection across layouts. Clear the offset
  // and refresh its content size if a previously projected viewport becomes
  // unbounded.
  if (!scroll_state_.measured() || (!bounded_viewport_ && !has_projection_) ||
      IsDetached() || will_destroy()) {
    return;
  }
  const bool physical_reverse = reverse_ != (!vertical_ && rtl_);
  const float physical_offset =
      bounded_viewport_ ? scroll_state_.physical_offset(physical_reverse) : 0.f;
  const float x = vertical_ ? 0 : physical_offset;
  const float y = vertical_ ? physical_offset : 0;
  has_projection_ = true;
  UpdateScrollOffset(x, y);
  // The create/update phase already queued native host creation.
  painting_context()->ApplyScrollViewState(
      impl_id(), x, y, projected_content_width_, projected_content_height_);
}

lepus::Value ModifierScrollElement::StateValue(double consumed_px,
                                               double remaining_px) {
  BASE_STATIC_STRING_DECL(kError, "error");
  BASE_STATIC_STRING_DECL(kValid, "valid");
  BASE_STATIC_STRING_DECL(kCarrierId, "carrierId");
  BASE_STATIC_STRING_DECL(kScrollOffsetPx, "scrollOffsetPx");
  BASE_STATIC_STRING_DECL(kViewportExtentPx, "viewportExtentPx");
  BASE_STATIC_STRING_DECL(kContentExtentPx, "contentExtentPx");
  BASE_STATIC_STRING_DECL(kCanScrollForward, "canScrollForward");
  BASE_STATIC_STRING_DECL(kCanScrollBackward, "canScrollBackward");
  BASE_STATIC_STRING_DECL(kRevision, "revision");
  BASE_STATIC_STRING_DECL(kConsumedPx, "consumedPx");
  BASE_STATIC_STRING_DECL(kRemainingPx, "remainingPx");
  const bool valid = scroll_state_.measured() && bounded_viewport_ &&
                     !IsDetached() && !will_destroy();
  const double scale =
      element_manager()->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit();
  auto state = lepus::Dictionary::Create();
  if (scroll_state_.measured() && !bounded_viewport_) {
    state->SetValue(kError,
                    "Scroll viewport is unbounded on its scrolling axis");
  }
  state->SetValue(kValid, valid);
  state->SetValue(kCarrierId, impl_id());
  state->SetValue(kScrollOffsetPx, scroll_state_.offset() * scale);
  state->SetValue(kViewportExtentPx, scroll_state_.viewport() * scale);
  state->SetValue(kContentExtentPx, scroll_state_.content() * scale);
  state->SetValue(kCanScrollForward,
                  valid && scroll_state_.offset() < scroll_state_.max_offset());
  state->SetValue(kCanScrollBackward, valid && scroll_state_.offset() > 0);
  state->SetValue(kRevision, static_cast<double>(scroll_state_.revision()));
  state->SetValue(kConsumedPx, consumed_px);
  state->SetValue(kRemainingPx, remaining_px);
  return lepus::Value(state);
}

lepus::Value ModifierScrollElement::InvokeMethod(const base::String& method,
                                                 const lepus::Value& params,
                                                 std::string* error) {
  if (method != "scrollCommand") {
    return ScrollElement::InvokeMethod(method, params, error);
  }
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kDeltaPx, "deltaPx");
  BASE_STATIC_STRING_DECL(kOffsetPx, "offsetPx");
  const auto request = params.ToLepusValue(true);
  const auto type = request.GetProperty(kType);
  if (type.IsString() && type.StdString() == "getState") return StateValue();
  const bool by = type.IsString() && type.StdString() == "scrollBy";
  const bool to = type.IsString() && type.StdString() == "scrollTo";
  const auto value = request.GetProperty(by ? kDeltaPx : kOffsetPx);
  if ((!by && !to) || !value.IsNumber() || !std::isfinite(value.Number()) ||
      std::abs(value.Number()) > std::numeric_limits<float>::max()) {
    if (error) *error = "Invalid modifier scroll command";
    return StateValue();
  }
  if (!scroll_state_.measured() || !bounded_viewport_ || IsDetached() ||
      will_destroy()) {
    return StateValue(0, by ? value.Number() : 0);
  }
  const double scale =
      element_manager()->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit();
  const double requested_px = value.Number();
  const double target =
      requested_px / scale + (by ? scroll_state_.offset() : 0);
  double consumed = scroll_state_.ScrollTo(target) * scale;
  if (consumed != 0) {
    Project();
    // Scroll-only script callbacks can finish without a layout pipeline.
    element_manager()->ScheduleUIOperationFlush();
  }
  if (by) {
    // Rounding must not send a reverse delta to the nested scroll parent.
    consumed = std::clamp(consumed, std::min(0., requested_px),
                          std::max(0., requested_px));
  }
  return StateValue(consumed, by ? requested_px - consumed : 0);
}

}  // namespace tasm
}  // namespace lynx
