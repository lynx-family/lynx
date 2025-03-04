// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_SELECTOR_ELEMENT_SELECTOR_H_
#define CORE_RENDERER_DOM_SELECTOR_ELEMENT_SELECTOR_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "core/renderer/dom/selector/select_result.h"
#include "core/renderer/dom/selector/selector_item.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"

namespace lynx {
namespace tasm {

class RadonBase;
class LynxGetUIResult;
class SelectElementToken;

class ElementSelector {
 protected:
  class SelectImplOptions {
   public:
    explicit SelectImplOptions(const NodeSelectOptions &options)
        : is_root_component(true),
          first_only(options.first_only),
          only_current_component(options.only_current_component),
          component_only(options.component_only),
          no_descendant(false) {}

    // the children of root component of searching should always be searched in
    bool is_root_component;
    // only return the first result
    bool first_only;
    // whether children of child components would be searched in
    bool only_current_component;
    // whether only components are collected as result
    bool component_only;
    // won't search in any children of this node
    bool no_descendant;
    // for searching in slot in fiber element tree
    std::string parent_component_id;
  };

  void Distribute(SelectorItem *root, const NodeSelectOptions &options);

  SelectImplOptions PrepareNextSelectOptions(const SelectElementToken &token,
                                             const SelectImplOptions &options,
                                             size_t token_pos,
                                             size_t next_token_pos);

  void SelectByCssSelector(SelectorItem *root,
                           const NodeSelectOptions &options);

  void SelectByRefId(SelectorItem *root, const NodeSelectOptions &options);

  virtual void SelectByElementId(SelectorItem *root,
                                 const NodeSelectOptions &options) = 0;

  virtual void SelectImpl(SelectorItem *adaptor,
                          const std::vector<SelectElementToken> &tokens,
                          size_t token_pos,
                          const SelectImplOptions &options) = 0;

  virtual void InsertResult(SelectorItem *base) = 0;
  virtual bool FoundElement() = 0;

  bool identifier_legal_ = true;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_SELECTOR_ELEMENT_SELECTOR_H_
