// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/selector/element_selector.h"

#include "base/include/string/string_utils.h"
#include "core/renderer/css/select_element_token.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"

namespace lynx {
namespace tasm {

void ElementSelector::Distribute(SelectorItem* root,
                                 const NodeSelectOptions& options) {
  switch (options.identifier_type) {
    case NodeSelectOptions::IdentifierType::CSS_SELECTOR:
      SelectByCssSelector(root, options);
      break;
    case NodeSelectOptions::IdentifierType::REF_ID:
      SelectByRefId(root, options);
      break;
    case NodeSelectOptions::IdentifierType::ELEMENT_ID: {
      SelectByElementId(root, options);
      break;
    }
  }
}

ElementSelector::SelectImplOptions ElementSelector::PrepareNextSelectOptions(
    const SelectElementToken& token,
    const ElementSelector::SelectImplOptions& options, size_t token_pos,
    size_t next_token_pos) {
  auto next_options = options;
  next_options.is_root_component = false;
  if (next_token_pos != token_pos) {
    next_options.only_current_component = token.OnlyCurrentComponent();
    next_options.no_descendant = token.NoDescendant();
    next_options.parent_component_id.clear();
  }
  return next_options;
}

void ElementSelector::SelectByRefId(SelectorItem* root,
                                    const NodeSelectOptions& options) {
  SelectImpl(
      root,
      {SelectElementToken(options.identifier, SelectElementToken::Type::REF_ID,
                          SelectElementToken::Combinator::LAST)},
      0, SelectImplOptions(options));
}

void ElementSelector::SelectByCssSelector(SelectorItem* root,
                                          const NodeSelectOptions& options) {
  if (options.identifier.empty()) {
    InsertResult(root);  // for compatibility
    identifier_legal_ = true;
    return;
  }

  std::vector<std::string> selector_vec;
  base::SplitString(options.identifier, ',', selector_vec);

  for (const auto& s : selector_vec) {
    auto pair = SelectElementToken::ParseCssSelector(s);
    identifier_legal_ = pair.second;
    if (!identifier_legal_) {
      return;
    }

    SelectImpl(root, pair.first, 0, SelectImplOptions(options));
    if (options.first_only && FoundElement()) {
      return;
    }
  }
}
}  // namespace tasm
}  // namespace lynx
