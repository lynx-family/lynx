// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "clay/shell/common/devtool/ui_tree_helper.h"

namespace lynx::tasm {

std::string UIDelegateClay::GetLynxUITree() {
  return ui_tree::GetLynxUITree(view_context_);
}

std::string UIDelegateClay::GetUINodeInfo(int id) {
  return ui_tree::GetUINodeInfo(view_context_, id);
}

int UIDelegateClay::SetUIStyle(int id, const std::string& name,
                               const std::string& content) {
  return ui_tree::SetUIStyle(view_context_, id, name, content);
}

}  // namespace lynx::tasm
