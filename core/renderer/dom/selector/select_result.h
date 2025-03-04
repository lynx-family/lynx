// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_SELECTOR_SELECT_RESULT_H_
#define CORE_RENDERER_DOM_SELECTOR_SELECT_RESULT_H_

#include <utility>
#include <vector>

#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

/*
 * `NodeSelectResult` represents the result of a node selection.
 *
 * It directly contains the result `nodes` of the original node type,
 * the input `options` of node selection, and `identifier_legal`
 * to tell if the input identifier(usually css selector) is legal.
 *
 * When used in `SelectorQuery`, a `NodeSelectResult` is usually converted to a
 * `LynxGetUIResult` by calling `PackageLynxGetUIResult`. A `LynxGetUIResult`
 * contains the result in type `Element`, and also error information needed to
 * be provided to the front-end users.
 */
template <typename Node>
struct NodeSelectResult {
  static inline int32_t GetImplId(Node *node);

  NodeSelectResult(std::vector<Node *> nodes, const NodeSelectOptions &options)
      : nodes(std::move(nodes)), options(options){};

  NodeSelectResult(std::vector<Node *> nodes, const NodeSelectOptions &options,
                   bool identifier_legal)
      : nodes(std::move(nodes)),
        options(options),
        identifier_legal(identifier_legal){};

  NodeSelectResult(const NodeSelectResult &other) = delete;
  NodeSelectResult &operator=(const NodeSelectResult &other) = delete;

  NodeSelectResult(NodeSelectResult &&other) = default;
  NodeSelectResult &operator=(NodeSelectResult &&other) = default;

  Node *GetOneNode() const { return nodes.empty() ? nullptr : nodes.front(); }

  bool Success() const { return identifier_legal && !nodes.empty(); }

  LynxGetUIResult PackageLynxGetUIResult() const {
    std::vector<int32_t> ui_impl_id_vector;
    if (!identifier_legal) {
      return LynxGetUIResult({}, LynxGetUIResult::SELECTOR_NOT_SUPPORTED,
                             options.NodeIdentifierMessage());
    }
    if (!root_found) {
      return LynxGetUIResult({}, LynxGetUIResult::NODE_NOT_FOUND,
                             options.NodeIdentifierMessage(),
                             "root node not found with identifier = " +
                                 options.NodeIdentifierMessage());
    }
    if (nodes.empty()) {
      return LynxGetUIResult(std::move(ui_impl_id_vector),
                             LynxGetUIResult::NODE_NOT_FOUND,
                             options.NodeIdentifierMessage());
    }

    for (auto node : nodes) {
      int32_t id = GetImplId(node);
      // ignore no ui nodes
      if (id == kInvalidImplId) {
        continue;
      }
      ui_impl_id_vector.push_back(id);
    }

    // all nodes in `nodes` do not have lynx ui
    if (ui_impl_id_vector.empty()) {
      return LynxGetUIResult({}, LynxGetUIResult::NO_UI_FOR_NODE,
                             options.NodeIdentifierMessage());
    }
    return LynxGetUIResult(std::move(ui_impl_id_vector),
                           LynxGetUIResult::SUCCESS,
                           options.NodeIdentifierMessage());
  }

  std::vector<Node *> nodes;
  NodeSelectOptions options;
  bool identifier_legal = true;
  bool root_found = true;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_SELECTOR_SELECT_RESULT_H_
