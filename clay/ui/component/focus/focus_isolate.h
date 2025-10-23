// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FOCUS_FOCUS_ISOLATE_H_
#define CLAY_UI_COMPONENT_FOCUS_FOCUS_ISOLATE_H_

#include <string>

#include "clay/ui/component/view.h"
#include "clay/ui/event/focus_manager.h"

namespace clay {

class PageView;

class FocusIsolate : public WithTypeInfo<FocusIsolate, View>,
                     public FocusNode::LifecycleListener {
 public:
  FocusIsolate(int id, PageView* page_view) : WithTypeInfo(id, page_view) {
    SetIsFocusScope();
    SetIsFocusFence();
  }

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  FocusManager::TraversalResult OnTraversalOnScope(
      FocusManager::Direction direction,
      FocusManager::TraversalType traversal_type) override;

  void OnFocusEscaping(FocusManager::Direction direction) override;
  void OnFocusEntering(FocusManager::Direction direction) override;

  void SetHasDefaultFocusRing(bool has_focus_ring) override {}

  void UpdateLastFocusChild(FocusNode* last_focused_view) {
    if (last_focused_view_) {
      last_focused_view_->RemoveLifecycleListener(this);
    }
    last_focused_view_ = last_focused_view;
    if (last_focused_view_) {
      last_focused_view_->AddLifecycleListener(this);
    }
  }

  bool allow_escape() override { return allow_escape_; }

 protected:
  void OnFocusNodeDestructed(FocusNode*) override;

 private:
  bool allow_escape_ = false;
  bool save_last_focus_child_ = true;
  // There's risk that a FocusNode has been destroyed and another FocusNode
  // created using same pointer value.
  FocusNode* last_focused_view_ = nullptr;
  std::string preferred_child_;

  FRIEND_TEST(FocusIsolateTest, AllowEscape);
  FRIEND_TEST(FocusIsolateTest, NestedScope);
  FRIEND_TEST(FocusIsolateTest, AutoRestoreFocusAcrossScope);
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_FOCUS_FOCUS_ISOLATE_H_
