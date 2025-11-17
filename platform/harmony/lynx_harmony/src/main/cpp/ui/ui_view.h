// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_VIEW_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_VIEW_H_

#include <string>
#include "core/base/lynx_export.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LYNX_EXPORT UIView : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag);

  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;

  void OnNodeReady() override;

  void OnNodeEvent(ArkUI_NodeEvent* event) override;

 protected:
  bool DefaultOverflowValue() override { return true; }
  UIView(LynxContext* context, ArkUI_NodeType type, int sign, const std::string& tag);
  ~UIView() override;

 private:
  bool is_consume_event_{false};

  bool is_root_attached_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_VIEW_H_
