// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_H_
#define CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include "clay/ui/component/scroll_coordinator/scroll_coordinator_scroll_view.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_toolbar.h"

namespace clay {

// The hierarchy of ScrollCoordinator:
// - ScrollCoordinator
//   - ScrollCoordinatorToolbar (optional)
//   // This node does not exist in the element tree. We use it to simplify the
//   // toolbar implementation.
//   - ScrollCoordinatorScrollView (auto-generated)
//     - ScrollCoordinatorHeader (required)
//     - ScrollCoordinatorSlot (required)
//       - ScrollCoordinatorSlotDrag (optional)
//       - Content (required, list/scroll-view/viewpager/...)
class ScrollCoordinator : public WithTypeInfo<ScrollCoordinator, BaseView> {
 public:
  ScrollCoordinator(int32_t id, PageView* page_view);
  ~ScrollCoordinator() override;

  void AddChild(BaseView* child, int index) override;
  void RemoveChild(BaseView* child) override;

  void OnLayout(LayoutContext* context) override;
  void AddEventCallback(const char* event) override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void setFoldExpanded(const LynxModuleValues& args,
                       const LynxUIMethodCallback& callback);
  bool IsLayoutRootCandidate() const override { return true; }

  ScrollCoordinatorScrollView* GetScrollView() const { return scroll_view_; }

 private:
  ScrollCoordinatorToolbar* toolbar_ = nullptr;
  ScrollCoordinatorScrollView* scroll_view_ = nullptr;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_SCROLL_COORDINATOR_SCROLL_COORDINATOR_H_
