// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_VDOM_RADON_NODE_SELECTOR_H_
#define CORE_RENDERER_DOM_VDOM_RADON_NODE_SELECTOR_H_

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/dom/selector/element_selector.h"
#include "core/renderer/dom/selector/select_result.h"
#include "core/renderer/dom/selector/selector_item.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"

namespace lynx {
namespace tasm {

class RadonComponent;
class RadonPage;
class LynxGetUIResult;
class SelectElementToken;
class SelectorItem;

using RadonNodeSelectResult = NodeSelectResult<RadonNode>;

template <>
inline int32_t NodeSelectResult<RadonNode>::GetImplId(RadonNode *node) {
  return node ? node->ImplId() : kInvalidImplId;
}

class RadonNodeSelector : public ElementSelector {
 public:
  BASE_EXPORT_FOR_DEVTOOL static RadonNodeSelectResult Select(
      RadonBase *root, const NodeSelectOptions &options);
  BASE_EXPORT_FOR_DEVTOOL static RadonNodeSelectResult Select(
      RadonPage *page, const NodeSelectRoot &root,
      const NodeSelectOptions &options);

 private:
  virtual void SelectImpl(SelectorItem *element_base,
                          const std::vector<SelectElementToken> &tokens,
                          size_t token_pos,
                          const SelectImplOptions &options) override;
  bool IsTokenSatisfied(RadonBase *base, const SelectElementToken &token);
  void SelectInSlots(RadonComponent *component,
                     const std::vector<SelectElementToken> &tokens, size_t pos,
                     const SelectImplOptions &options);
  virtual void SelectByElementId(SelectorItem *root,
                                 const NodeSelectOptions &options) override;

  virtual void InsertResult(SelectorItem *element_base) override;
  virtual bool FoundElement() override;
  void UniqueAndSortResult(RadonBase *root);

  std::vector<RadonNode *> result_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_VDOM_RADON_NODE_SELECTOR_H_
