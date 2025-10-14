// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SEMANTICS_SEMANTICS_OWNER_H_
#define CLAY_UI_SEMANTICS_SEMANTICS_OWNER_H_

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/ui/semantics/semantics_node.h"
#include "clay/ui/semantics/semantics_update_node.h"

namespace clay {
class BaseView;
class PageView;

class SemanticsOwner {
 public:
  explicit SemanticsOwner(
      std::function<void(const SemanticsUpdateNodes& update_nodes)>
          update_callback)
      : on_semantics_update_(update_callback) {}

  void SendSemanticsUpdate();

  void AddDirtySemanticsForDescendants(BaseView* node);
  void RemoveDirtySemanticsForDescendants(BaseView* node);

  void SetSemanticsEnabled(bool enabled) {
    if (enabled != semantics_enabled_) {
      semantics_enabled_ = enabled;
    }
  }
  bool IsSemanticsEnabled() const { return semantics_enabled_; }

  void SetRebuildSemanticsTree(bool value) {
    if (semantics_enabled_ && need_rebuild_semantics_tree_ != value) {
      need_rebuild_semantics_tree_ = value;
    }
  }
  bool NeedRebuildSemanticsTree() const { return need_rebuild_semantics_tree_; }

  // Return whether this value changes.
  bool SetPageEnableAccessibilityElement(bool enabled) {
    if (page_enable_accessibility_element_ != enabled) {
      page_enable_accessibility_element_ = enabled;
      need_rebuild_semantics_tree_ = true;
      return true;
    }
    return false;
  }

  bool IsPageEnableAccessibilityElement() const {
    return page_enable_accessibility_element_;
  }

  BaseView* GetViewFromId(int id) const;

  void Reset();

 private:
  friend class SemanticsNode;
  friend class PageView;

  std::unordered_map<int, fml::RefPtr<SemanticsNode>> nodes_;
  std::unordered_set<fml::RefPtr<SemanticsNode>> dirty_nodes_;
  std::unordered_set<fml::RefPtr<SemanticsNode>> detached_nodes_;

  std::function<void(SemanticsUpdateNodes)> on_semantics_update_;

  // Set that contains all the nodes that need update semantics of both
  // themselves and all descendants.
  std::unordered_set<BaseView*> semantics_nodes_to_update_descendants_;
  bool semantics_enabled_ = false;
  bool page_enable_accessibility_element_ = true;
  bool need_rebuild_semantics_tree_ = true;
};

}  // namespace clay

#endif  // CLAY_UI_SEMANTICS_SEMANTICS_OWNER_H_
