// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_SELECTOR_FIBER_ELEMENT_SELECTOR_H_
#define CORE_RENDERER_DOM_SELECTOR_FIBER_ELEMENT_SELECTOR_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/base/lynx_export.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/dom/selector/element_selector.h"
#include "core/renderer/dom/selector/select_result.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"

namespace lynx {
namespace tasm {

class LynxGetUIResult;
class SelectElementToken;

template <>
inline int32_t NodeSelectResult<Element>::GetImplId(Element *node) {
  return node ? node->impl_id() : kInvalidImplId;
}

class FiberElementSelector : public ElementSelector {
 public:
  using ElementSelectResult = NodeSelectResult<Element>;

  LYNX_EXPORT_FOR_DEVTOOL static ElementSelectResult Select(
      Element *root, const NodeSelectOptions &options);
  LYNX_EXPORT_FOR_DEVTOOL static ElementSelectResult Select(
      const std::unique_ptr<ElementManager> &element_manager,
      const NodeSelectRoot &root, const NodeSelectOptions &options);

 private:
  virtual void SelectImpl(SelectorItem *base,
                          const std::vector<SelectElementToken> &tokens,
                          size_t token_pos,
                          const SelectImplOptions &options) override;
  void SelectImplRecursive(Element *element,
                           const std::vector<SelectElementToken> &tokens,
                           size_t token_pos, const SelectImplOptions &options);
  bool IsTokenSatisfied(Element *base, const SelectElementToken &token);

  virtual void SelectByElementId(SelectorItem *root,
                                 const NodeSelectOptions &options) override;

  virtual void InsertResult(SelectorItem *base) override;
  virtual bool FoundElement() override;

  void SelectInSlots(Element *element,
                     const std::vector<SelectElementToken> &tokens,
                     size_t token_pos, const SelectImplOptions &options,
                     const std::string &parent_component_id);

  void UniqueAndSortResult(Element *root);

  std::vector<Element *> result_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_SELECTOR_FIBER_ELEMENT_SELECTOR_H_
