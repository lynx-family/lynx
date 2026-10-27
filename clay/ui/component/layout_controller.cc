// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/layout_controller.h"

#include <unordered_set>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "core/base/trace/trace_event_def.h"

namespace clay {

PreLayoutContext::PreLayoutContext() = default;
PreLayoutContext::~PreLayoutContext() = default;

LayoutContext::LayoutContext() = default;
LayoutContext::~LayoutContext() = default;

LayoutController::LayoutController() = default;
LayoutController::~LayoutController() = default;

void LayoutController::AddNeedLayout(fml::WeakPtr<BaseView> node_weak) {
  if (node_weak.get() == nullptr) {
    return;
  }
  nodes_needing_layout_.insert(node_weak);
}

void LayoutController::Layout() {
  if (nodes_needing_layout_.empty()) {
    return;
  }
  std::unordered_set<fml::WeakPtr<BaseView>, BaseViewWeakPtrHash> dirty_nodes;
  dirty_nodes.swap(nodes_needing_layout_);
  TRACE_EVENT("clay", CLAY_LAYOUT_CONTROLLER_LAYOUT, "dirty_root_count",
              dirty_nodes.size());
  for (fml::WeakPtr<BaseView> layout_root_weak : dirty_nodes) {
    BaseView* layout_root = layout_root_weak.get();
    if (layout_root == nullptr) {
      continue;
    }
    TRACE_EVENT("clay", CLAY_LAYOUT_CONTROLLER_LAYOUT_DIRTY_ROOT, "view_id",
                layout_root->GetCallbackId(), "tag",
                layout_root->GetName().c_str());
    layout_root->Layout(nullptr);
  }
}

}  // namespace clay
