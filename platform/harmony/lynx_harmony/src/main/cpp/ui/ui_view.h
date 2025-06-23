// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_VIEW_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_VIEW_H_

#include <string>
#include "base/include/base_export.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class BASE_EXPORT UIView : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag);

  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;

 protected:
  bool DefaultOverflowValue() override { return true; }
  UIView(LynxContext* context, ArkUI_NodeType type, int sign, const std::string& tag);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_VIEW_H_
