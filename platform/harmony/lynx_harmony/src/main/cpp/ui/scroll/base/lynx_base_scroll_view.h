// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_H_

#include <arkui/native_node.h>
#include <arkui/native_type.h>

#include <string>
#include <utility>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view_auto.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view_fundamental.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view_horizontal.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view_nested.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view_vertical.h"

namespace lynx {
namespace tasm {
namespace harmony {

enum class LynxBaseScrollViewScrollState {
  LynxBaseScrollViewScrollStateIdle = 0,
  LynxBaseScrollViewScrollStateDragging,
  LynxBaseScrollViewScrollStateAnimating,
  LynxBaseScrollViewScrollStateFling,
};

class LynxBaseScrollViewDelegate {
 public:
  virtual void OnScrollStateChanged(LynxBaseScrollViewScrollState from,
                                    LynxBaseScrollViewScrollState to) = 0;
  virtual void ScrollViewDidScroll() = 0;
  virtual ~LynxBaseScrollViewDelegate() = default;
};

class LynxBaseScrollView : public LynxBaseScrollViewVertical,
                           public LynxBaseScrollViewHorizontal,
                           public LynxBaseScrollViewAuto,
                           public LynxBaseScrollViewNested,
                           public LynxBaseScrollViewFundamental,
                           std::enable_shared_from_this<LynxBaseScrollView> {
 public:
  explicit LynxBaseScrollView(LynxBaseScrollViewDelegate* delegate);

  ~LynxBaseScrollView() override;

  float GetScrollOffsetVertically() override;

  void SetScrollContentSizeVertically(float content_size) override;

  void ScrollByUnlimitedVertically(float delta) override;

  void ScrollByVertically(float delta) override;

  void ScrollToUnlimitedVertically(float offset) override;

  void ScrollToVertically(float offset) override;

  void AnimatedScrollToVertically(float offset) override;

  void AnimatedScrollToUnlimitedVertically(float offset) override;

  void GetScrollRangeVertically(float range[2]) override;

  bool CanScrollForwardsVertically() override;

  bool CanScrollBackwardsVertically() override;

  float GetScrollOffsetHorizontally() override;

  void SetScrollContentSizeHorizontally(float content_size) override;

  void ScrollByUnlimitedHorizontally(float delta) override;

  void ScrollByHorizontally(float delta) override;

  void ScrollToUnlimitedHorizontally(float offset) override;

  void ScrollToHorizontally(float offset) override;

  void AnimatedScrollToHorizontally(float offset) override;

  void AnimatedScrollToUnlimitedHorizontally(float offset) override;

  void GetScrollRangeHorizontally(float range[2]) override;

  bool CanScrollForwardsHorizontally() override;

  bool CanScrollBackwardsHorizontally() override;

  void GetScrollOffset(float scroll_offset[2]) override;

  void SetScrollContentSize(float content_size[2]) override;

  void ScrollByUnlimited(float delta[2]) override;

  void ScrollBy(float delta[2]) override;

  void ScrollToUnlimited(float offset[2]) override;

  void ScrollTo(float offset[2]) override;

  void AnimatedScrollTo(float offset[2]) override;

  void AnimatedScrollToUnlimited(float offset[2]) override;

  void GetScrollRange(float range[4]) override;

  bool CanScrollForwards() override;

  bool CanScrollBackwards() override;

  NestedScrollMode ForwardsNestedScrollMode() override;

  NestedScrollMode BackwardsNestedScrollMode() override;

  void SetForwardsNestedScrollMode(NestedScrollMode mode) override;

  void SetBackwardsNestedScrollMode(NestedScrollMode mode) override;

  void SetVertical(bool vertical) override;

  bool Vertical() override;

  void EnableScroll(bool enable) override;

  bool ScrollEnabled() override;

  void StopScrolling() override;

  void SetBounces(bool bounces) override;

  bool Bounces() override;

  void EnableScrollBar(bool enable) override;

  bool Dragging() override;

  int CurrentScrollState() override;

  void TryToUpdateScrollState(LynxBaseScrollViewScrollState newState);

  ArkUI_NodeHandle node_{nullptr};
  ArkUI_NodeHandle scroll_content_{nullptr};

 protected:
  static void EventReceiver(ArkUI_NodeEvent* event);
  float content_size_[2]{.0f};
  bool dragging_{false};
  LynxBaseScrollViewDelegate* delegate_{nullptr};
  LynxBaseScrollViewScrollState scroll_state_{
      LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateIdle};
  NestedScrollMode forwards_nested_scroll_mode_{
      NestedScrollMode::NestedScrollModeSelfFirst};
  NestedScrollMode backwards_nested_scroll_mode_{
      NestedScrollMode::NestedScrollModeSelfFirst};

 private:
  void OnScroll(ArkUI_NodeEvent* event);
  void OnScrollStart(ArkUI_NodeEvent* event);
  void OnScrollStop(ArkUI_NodeEvent* event);
  void OnScrollEdge(ArkUI_NodeEvent* event);
  void OnWillScroll(ArkUI_NodeEvent* event);
  void OnDidScroll(ArkUI_NodeEvent* event);
  void OnTouchEvent(ArkUI_NodeEvent* event);

  bool vertical_{true};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_BASE_LYNX_BASE_SCROLL_VIEW_H_
