// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/overlay_view.h"

#include "clay/fml/logging.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/overlay_manager.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/rendering/render_overlay.h"

namespace clay {

OverlayView::OverlayView(uint32_t id, PageView* page_view)
    : WithTypeInfo(id, "overlay-view", std::make_unique<RenderOverlay>(),
                   page_view) {
  overlay_manager_ = page_view->overlay_manager();
  overlay_id_ = (overlay_manager_->GenerateNextOverlayId());
  render_object_->SetOverlay(true);
  SetIsFocusScope();
  SetIsFocusFence();
}
OverlayView::OverlayView(uint32_t id, const std::string& tag,
                         PageView* page_view)
    : WithTypeInfo(id, tag, std::make_unique<RenderOverlay>(), page_view) {
  overlay_manager_ = page_view->overlay_manager();
  overlay_id_ = overlay_manager_->GenerateNextOverlayId();
  render_object_->SetOverlay(true);
  SetIsFocusScope();
  SetIsFocusFence();
}

OverlayView::~OverlayView() = default;

void OverlayView::SetPassEventsThrough(bool pass_events_through) {
  // NOTE(hanhaoshen): Currently we only support switch pass_events_through to
  // false once.
  if (!pass_events_through && pass_events_through_) {
    pass_events_through_ = pass_events_through;
    focus_manager_->SetIsRootScope();
    overlay_manager_->SaveFocus();
  }
}

void OverlayView::SetFullScreen(bool full_screen) {
  // NOTE(hanhaoshen): Currently we only support switch full_screen to true
  // once.
  if (full_screen && !full_screen_) {
    full_screen_ = true;
    SetX(0);
    SetY(0);
    SetWidth(page_view_->Width());
    SetHeight(page_view_->Height());
  }
}

void OverlayView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kVisible) {
    bool visible = attribute_utils::GetBool(value);
    visible ? Show() : Hide();
  } else if (kw == KeywordID::kEventsPassThrough) {
    SetPassEventsThrough(attribute_utils::GetBool(value));
  } else if (kw == KeywordID::kOverlayId) {
    std::string new_overlay_id = attribute_utils::GetCString(value);
    overlay_id_ = new_overlay_id;
  } else if (kw == KeywordID::kFullScreen) {
    SetFullScreen(attribute_utils::GetBool(value));
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void OverlayView::SetBound(float left, float top, float width, float height) {
  if (full_screen_) {
    return;
  }

  BaseView::SetBound(left, top, width, height);
}

void OverlayView::Hide() {
  if (!Visible()) {
    return;
  }
  FML_DCHECK(overlay_manager_);
  SetVisible(false);
  if (attach_to_tree_) {
    SetFocusable(false);
    overlay_manager_->OnHideOverlay(this);
  }
}

void OverlayView::Show() {
  if (Visible()) {
    return;
  }
  FML_DCHECK(overlay_manager_);
  SetVisible(true);
  if (attach_to_tree_) {
    SetFocusable(true);
    overlay_manager_->OnShowOverlay(this);
  }
}

bool OverlayView::HitTest(const PointerEvent& event, HitTestResult& result,
                          bool& is_pass_through) {
  bool ret = BaseView::HitTest(event, result);
  if (pass_events_through_ && !result.empty() && result.back().get() == this) {
    // When `events-pass-through` is enabled, hit testing is prevented on the
    // overlay itself.
    is_pass_through = true;
    result.pop_back();
    if (!result.empty()) {
      // Insert a null elememt as a boundary symbol. It will be checked
      // in gesture_manager after hittest.
      result.emplace_back();
    }
    return !result.empty();
  }
  if (!result.empty()) {
    // Insert a null elememt as a boundary symbol. It will be checked
    // in gesture_manager after hittest.
    result.emplace_back();
  }
  return ret;
}

bool OverlayView::HitTest(const PointerEvent& event, HitTestResult& result) {
  bool is_pass_through = false;
  return HitTest(event, result, is_pass_through);
}

void OverlayView::OnAttachToTree() {
  BaseView::OnAttachToTree();

  if (Visible()) {
    SetFocusable(true);
    overlay_manager_->OnShowOverlay(this);
  }
}

void OverlayView::OnDetachFromTree() {
  Hide();
  BaseView::OnDetachFromTree();
}

}  // namespace clay
