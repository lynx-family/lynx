// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_VIEWPAGER_UI_VIEWPAGER_ITEM_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_VIEWPAGER_UI_VIEWPAGER_ITEM_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIViewPagerItem : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIViewPagerItem(context, sign, tag);
  }

 protected:
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  bool NotifyParent() override { return true; };
  UIViewPagerItem(LynxContext* context, int sign, const std::string& tag);
  bool DefaultOverflowValue() override { return false; }
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_VIEWPAGER_UI_VIEWPAGER_ITEM_H_
