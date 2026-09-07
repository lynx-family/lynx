// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DEVTOOL_UI_TREE_HELPER_H_
#define CLAY_SHELL_COMMON_DEVTOOL_UI_TREE_HELPER_H_

#include <string>

namespace clay {
class ViewContext;
}

namespace lynx::tasm::ui_tree {

std::string GetLynxUITree(clay::ViewContext* view_context);
std::string GetUINodeInfo(clay::ViewContext* view_context, int id);
int SetUIStyle(clay::ViewContext* view_context, int id, const std::string& name,
               const std::string& content);

}  // namespace lynx::tasm::ui_tree

#endif  // CLAY_SHELL_COMMON_DEVTOOL_UI_TREE_HELPER_H_
