// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_COMPONENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_COMPONENT_H_
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIComponent : public UIView {
 public:
  class NodeReadyListener {
   public:
    virtual void OnComponentNodeReady(UIComponent* ui_component) = 0;
  };

  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIComponent(context, sign, tag);
  }

  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;

  const std::string& item_key() const { return item_key_; }

  void OnNodeReady() override;

  int z_index() const { return z_index_; }

  void SetNodeReadyListener(NodeReadyListener* node_ready_listener) {
    node_ready_listener_ = node_ready_listener;
  }

 protected:
  UIComponent(LynxContext* context, int sign, const std::string& tag);

 private:
  std::string item_key_;
  int z_index_{0};
  NodeReadyListener* node_ready_listener_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_COMPONENT_H_
