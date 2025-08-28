// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_BOUNCE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_BOUNCE_H_

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIBounce : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIBounce(context, sign, tag);
  }
  bool is_lower_{false};

 protected:
  UIBounce(LynxContext* context, int sign, const std::string& tag);
  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;
  bool DefaultOverflowValue() override { return false; }
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_BOUNCE_H_
