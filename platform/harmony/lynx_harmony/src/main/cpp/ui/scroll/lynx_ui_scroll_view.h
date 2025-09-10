// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_LYNX_UI_SCROLL_VIEW_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_LYNX_UI_SCROLL_VIEW_H_

#include <string>
#include <unordered_map>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/lynx_ui_scroll_view_internal.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxUIScrollView : public LynxUIScrollViewInternal {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new LynxUIScrollView(context, sign, tag);
  }

  using PropSetter = void (LynxUIScrollView::*)(const lepus::Value& value);
  using UIMethod = void (LynxUIScrollView::*)(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;
  void InvokeMethod(const std::string& name, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;

  std::unordered_map<std::string, PropSetter> prop_setters_ = {
      {"scroll-orientation", &LynxUIScrollViewInternal::SetScrollOrientation},
      {"enable-scroll-bar", &LynxUIScrollViewInternal::SetEnableScrollBar},
      {"enable-scroll", &LynxUIScrollViewInternal::SetEnableScroll},
      {"bounces", &LynxUIScrollViewInternal::SetBounces},
      {"forwards-nested-scroll",
       &LynxUIScrollViewInternal::SetForwardsNestedScroll},
      {"backwards-nested-scroll",
       &LynxUIScrollViewInternal::SetBackwardsNestedScroll},
      {"initial-scroll-index",
       &LynxUIScrollViewInternal::SetInitialScrollIndex},
      {"initial-scroll-offset",
       &LynxUIScrollViewInternal::SetInitialScrollOffset},
      {"upper-threshold", &LynxUIScrollViewInternal::SetUpperThreshold},
      {"lower-threshold", &LynxUIScrollViewInternal::SetLowerThreshold},
      {"scroll-event-throttle",
       &LynxUIScrollViewInternal::SetScrollEventThreshold},
  };
  std::unordered_map<std::string, UIMethod> ui_method_map_ = {
      {"scrollTo", &LynxUIScrollViewInternal::UIScrollTo},
      {"scrollBy", &LynxUIScrollViewInternal::UIScrollBy},
      {"autoScroll", &LynxUIScrollViewInternal::UIAutoScroll},
  };

 protected:
  LynxUIScrollView(LynxContext* context, int sign, const std::string& tag);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_SCROLL_LYNX_UI_SCROLL_VIEW_H_
