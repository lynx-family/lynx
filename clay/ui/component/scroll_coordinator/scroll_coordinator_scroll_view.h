// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SCROLL_VIEW_H_
#define CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SCROLL_VIEW_H_

#include <memory>
#include <string>

#include "clay/gfx/animation/viscous_fluid_interpolator.h"
#include "clay/public/value.h"
#include "clay/shell/common/services/gesture_mediate_service.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_header.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_slot.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_toolbar.h"
#include "clay/ui/component/scroller.h"

namespace clay {

class ScrollCoordinatorScrollView
    : public WithTypeInfo<ScrollCoordinatorScrollView, NestedScrollable>,
      public Scroller::Delegate {
 public:
  enum class State { FULLY_FOLD, FULLY_OPEN, OPEN };
  ScrollCoordinatorScrollView(int id, int callback_id, PageView* page_view);
  ~ScrollCoordinatorScrollView() override;

  int GetCallbackId() override { return callback_id_; }
  void AddChild(BaseView* child, int index) override;
  void RemoveChild(BaseView* child) override;
  void HandleEvent(const PointerEvent& event) override;

  void SetToolbarHeight(float height) { toolbar_height_ = height; }

  void AddEventCallback(const char* event) override;
  void OnLayout(LayoutContext* context) override;
  ScrollCoordinatorHeader* GetHeader() const;

  // this method doesn't include nested-scroll
  void OnScrollUpdate(float offset) override;
  State GetState() const;
  void SyncState();
  void SetFoldExpanded(float header_offset_px, bool with_anim);
  void SetAttribute(const char* attr, const clay::Value& value) override;
  int GetHeaderIndex() const { return head_over_slot_ ? child_count() : 0; }
  int GetSlotIndex() const { return head_over_slot_ ? 0 : child_count(); }

  bool CaptureScroll(FloatPoint delta) override;
  void OnChildSizeChanged(BaseView* child) override;
  FloatPoint DoScroll(FloatPoint delta, bool by_user_input = true,
                      bool ignore_repaint = false) override;

 private:
  void SetHeadOverSlot(bool head_over_slot);
  void DidScroll() override;

  // header is mandatory
  ScrollCoordinatorHeader* header_ = nullptr;
  // slot is mandatory
  ScrollCoordinatorSlot* slot_ = nullptr;

  std::unique_ptr<Scroller> scroller_;
  float max_scroll_offset_ = 0.f;
  float toolbar_height_ = 0.f;
  float last_sent_offset_y_ = 0.f;
  float granularity_ = 0.01f;
  bool head_over_slot_ = false;
  State state_ = State::FULLY_OPEN;
  int callback_id_ = -1;
  Puppet<Owner::kUI, GestureMediateService> gesture_mediate_puppet_;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_SCROLL_VIEW_H_
