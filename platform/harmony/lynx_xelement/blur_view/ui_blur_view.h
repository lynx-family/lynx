// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_BLUR_VIEW_UI_BLUR_VIEW_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_BLUR_VIEW_UI_BLUR_VIEW_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIBlurView : public UIBase {
 public:
  static UIBase *Make(LynxContext *context, int sign, const std::string &tag) {
    return new UIBlurView(context, sign, tag);
  }
  void OnPropUpdate(const std::string &name,
                    const lepus::Value &value) override;

 protected:
  UIBlurView(LynxContext *context, int sign, const std::string &tag);

 private:
  int api_version_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_BLUR_VIEW_UI_BLUR_VIEW_H_
