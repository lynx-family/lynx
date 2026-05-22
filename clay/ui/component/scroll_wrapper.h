// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLL_WRAPPER_H_
#define CLAY_UI_COMPONENT_SCROLL_WRAPPER_H_

#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/scrollbar/scrollbar_wrapper.h"
#include "clay/ui/lynx_module/types.h"

namespace clay {

class ScrollWrapper : public WithTypeInfo<ScrollWrapper, ScrollbarWrapper>,
                      public ScrollView::Delegate,
                      public Scrollable::Listener {
 public:
  ScrollWrapper(int id, PageView* page_view);
  ScrollWrapper(int id, ScrollDirection direction, PageView* page_view);
  ~ScrollWrapper() override;

  // UI Method
  void scrollTo(const LynxModuleValues& args);
  void autoScroll(const LynxModuleValues& args);
  void getScrollInfo(const LynxModuleValues& args,
                     const LynxUIMethodCallback& callback);

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  BaseView* GetTopViewToAcceptEvent(const FloatPoint& position,
                                    FloatPoint* relative_position) override;

  bool IsLayoutRootCandidate() const override { return true; }

  void SetDirection(int type) override;

#ifdef ENABLE_ACCESSIBILITY
  int32_t GetSemanticsActions() const override;
  int32_t GetSemanticsFlags() const override;
  int32_t GetA11yScrollChildren() const override;
  bool IsAccessibilityElement() const override;
  FloatRect GetSemanticsBounds() const override;
#endif

  ScrollView* GetScrollView() const { return static_cast<ScrollView*>(view_); }

 private:
  void OnDestroy() override;

  // Override ScrollbarWrapper
  void WillUpdateScrollbar() override;
  float GetScrollbarScrollOffset() override;
  float GetTotalLength() override;

  // Override ScrollView::Delegate
  void OnScrollViewChildAdded() override;
  void OnScrollViewChildRemoved() override;
  void OnScrollViewChildUpdated() override;

  // Override Scrollable::Listener
  void OnScrollableScrolled() override;

  // Override ScrollbarView::Delegate
  void OnScrollbarScrolled(float old_position, float new_position,
                           bool by_interaction, bool smooth) override;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SCROLL_WRAPPER_H_
