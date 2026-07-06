// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_FIBER_NODE_INFO_H_
#define CORE_RENDERER_DOM_FIBER_FIBER_NODE_INFO_H_

#include <string>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace tasm {
/*
 * `FiberNodeInfo` contains some utility functions to get some attributes of a
 * fiber node.
 */
class FiberNodeInfo {
 public:
  FiberNodeInfo() = delete;
  ~FiberNodeInfo() = delete;

  /**
   * Used by path() of SelectorQuery to get the nodes' required info.
   * @param nodes The nodes to get info.
   * @return The info of the nodes as lepus value.
   */
  static lepus::Value GetNodesInfo(const std::vector<Element *> &nodes,
                                   const std::vector<std::string> &fields);

  /**
   * Get node info by fields. Required info will be returned as lepus
   * dictionary.
   * @param node The node to get info.
   * @param fields fields to get.
   * @return A dictionary contains the information of the node as lepus value.
   */
  static lepus::Value GetNodeInfo(Element *node,
                                  const std::vector<std::string> &fields);

  static std::vector<Element *> PathToRoot(Element *base);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_FIBER_NODE_INFO_H_
