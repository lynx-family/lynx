// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_LYNX_UI_SCROLL_VIEW_INTERNAL_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_LYNX_UI_SCROLL_VIEW_INTERNAL_H_

#include <string>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view_auto_scroller.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxUIScrollViewInternal : public UIView, LynxBaseScrollViewDelegate {
 public:
  void SetScrollOrientation(const lepus::Value& value);
  void SetEnableScrollBar(const lepus::Value& value);
  void SetEnableScroll(const lepus::Value& value);
  void SetBounces(const lepus::Value& value);
  void SetForwardsNestedScroll(const lepus::Value& value);
  void SetBackwardsNestedScroll(const lepus::Value& value);
  void SetInitialScrollIndex(const lepus::Value& value);
  void SetInitialScrollOffset(const lepus::Value& value);
  void SetUpperThreshold(const lepus::Value& value);
  void SetLowerThreshold(const lepus::Value& value);
  void SetScrollEventThreshold(const lepus::Value& value);

  void UIScrollTo(const lepus::Value& args,
                  base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void UIScrollBy(const lepus::Value& args,
                  base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void UIAutoScroll(const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  LynxBaseScrollView* scroll_view_{nullptr};

 protected:
  LynxUIScrollViewInternal(LynxContext* context, int sign, const std::string& tag);
  ~LynxUIScrollViewInternal() override;
  void OnScrollStateChanged(LynxBaseScrollViewScrollState from,
                            LynxBaseScrollViewScrollState to) override;
  void ScrollViewDidScroll() override;
  void InsertNode(UIBase* child, int index) override;
  void RemoveNode(UIBase* child) override;
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  void FinishLayoutOperation() override;
  bool IsScrollable() override;
  float ScrollX() override;
  float ScrollY() override;
  void ScrollIntoView(bool smooth, const UIBase* target, const std::string& block,
                      const std::string& inline_value) override;

 private:
  static void GetPositionOf(float result[2], UIBase* target, LynxUIScrollViewInternal* scroll,
                            bool isRTL);
  static void AddOffset(float offset[2], float delta[2], bool is_rtl);
  static void FormatOffset(float offset[2], LynxUIScrollViewInternal* scroll, bool is_rtl);

  void FlushFirstRenderOperations();
  void SendScrollEvent(const char* name, lepus::Value params);
  void TryToSendScrollEvent();
  void UpdateScrollPosition();
  void UpdateStickyItems();

  LynxBaseScrollViewAutoScroller* auto_scroller_{nullptr};
  bool is_first_render_{true};
  bool at_upper_{false};
  bool at_lower_{false};
  float throttle_{0.f};
  float upper_threshold_{0.f};
  float lower_threshold_{0.f};
  float last_content_offset_[2]{0.f};
  std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration>
      last_update_time_{std::chrono::steady_clock::now()};
  std::vector<base::MoveOnlyClosure<void, LynxUIScrollViewInternal*>*>* first_render_block_array_{
      nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_LYNX_UI_SCROLL_VIEW_INTERNAL_H_
