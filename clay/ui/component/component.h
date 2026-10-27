// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_COMPONENT_H_
#define CLAY_UI_COMPONENT_COMPONENT_H_

#include "clay/ui/component/base_view.h"

namespace clay {

class Component : public WithTypeInfo<Component, BaseView> {
 public:
  class NodeReadyListener {
   public:
    virtual void OnComponentNodeReady(Component* component) = 0;
  };

  Component(int id, PageView* page_view);
  ~Component() override;
  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  void SetNodeReadyListener(NodeReadyListener* node_ready_listener) {
    node_ready_listener_ = node_ready_listener;
  }

  std::optional<int> GetZIndex() const { return z_index_; }

  void OnNodeReady() override;

 private:
  // for normal base_view, the z-order has been set by the lynx side (the order
  // that has been layout). But considering the ListView, the order of child
  // should be the index.
  std::optional<int> z_index_;
  NodeReadyListener* node_ready_listener_{nullptr};
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_COMPONENT_H_
