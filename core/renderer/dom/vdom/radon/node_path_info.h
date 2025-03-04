// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_NODE_PATH_INFO_H_
#define CORE_RENDERER_DOM_VDOM_RADON_NODE_PATH_INFO_H_

#include <string>
#include <vector>

#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class RadonPathInfo {
 public:
  RadonPathInfo() = delete;
  ~RadonPathInfo() = delete;

  // returns {"tag", "id", "dataSet", "index", "class"} of the given nodes.
  // this is used by SelectorQuery Path() ability.
  static lepus::Value GetNodesInfo(const std::vector<RadonNode *> &nodes);
  static lepus::Value GetNodeInfo(RadonNode *node,
                                  const std::vector<std::string> &fields);
  static std::vector<RadonNode *> PathToRoot(RadonBase *base);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_NODE_PATH_INFO_H_
