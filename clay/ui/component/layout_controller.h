// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LAYOUT_CONTROLLER_H_
#define CLAY_UI_COMPONENT_LAYOUT_CONTROLLER_H_

#include <unordered_set>

#include "base/include/fml/memory/weak_ptr.h"

namespace clay {

class BaseView;

// Context that for pre-layout phase. It should be overriden so that view can
// collect information from subviews.
class PreLayoutContext {
 public:
  PreLayoutContext();
  virtual ~PreLayoutContext();
};

// Layout Context that should be overriden by views so that the parent can pass
// some information to its subviews.
class LayoutContext {
 public:
  LayoutContext();
  virtual ~LayoutContext();
};

class LayoutController {
 public:
  LayoutController();
  ~LayoutController();

  // To simplify things, the view should call Invalidate() itself.
  void Layout();
  void AddNeedLayout(fml::WeakPtr<BaseView> node_weak);

  bool RemoveDirtyNode(fml::WeakPtr<BaseView> node_weak) {
    return nodes_needing_layout_.erase(node_weak) == 1;
  }

  bool HasDirtyNodes() const { return !nodes_needing_layout_.empty(); }

  struct BaseViewWeakPtrHash {
    size_t operator()(const fml::WeakPtr<clay::BaseView>& key) const {
      auto base_view = key.get();
      return std::hash<void*>{}((base_view));
    }
  };

 private:
  std::unordered_set<fml::WeakPtr<BaseView>, BaseViewWeakPtrHash>
      nodes_needing_layout_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LAYOUT_CONTROLLER_H_
