// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/selector/fiber_element_selector.h"

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>

#include "base/include/string/string_number_convert.h"
#include "base/include/vector.h"
#include "core/renderer/css/select_element_token.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace tasm {

FiberElementSelector::ElementSelectResult FiberElementSelector::Select(
    FiberElement* root, const NodeSelectOptions& options) {
  if (root == nullptr) {
    ElementSelectResult result{{}, options};
    result.root_found = false;
    return result;
  }
  LOGI(" SelectNode: " << options.ToString()
                       << ", root_impl_id: " << root->impl_id());
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElementSelector::Select");
  FiberElementSelector selector;
  selector.Distribute(root, options);
  selector.UniqueAndSortResult(root);
  return ElementSelectResult{std::move(selector.result_), options,
                             selector.identifier_legal_};
}

FiberElementSelector::ElementSelectResult FiberElementSelector::Select(
    const std::unique_ptr<ElementManager>& element_manager,
    const NodeSelectRoot& root, const NodeSelectOptions& options) {
  LOGI(" SelectNodeRoot: " << root.ToPrettyString());
  FiberElement* base = nullptr;
  switch (root.root_type) {
    case NodeSelectRoot::RootType::COMPONENT_ID:
      base = static_cast<FiberElement*>(
          element_manager->GetComponent(root.component_id));
      break;
    case NodeSelectRoot::RootType::NODE_UNIQUE_ID:
      base = static_cast<FiberElement*>(
          element_manager->node_manager()->Get(root.node_unique_id));
      break;
  }
  return Select(base, options);
}

void FiberElementSelector::SelectImpl(
    SelectorItem* base, const std::vector<SelectElementToken>& tokens,
    size_t token_pos, const SelectImplOptions& options) {
  FiberElement* element = static_cast<FiberElement*>(base);
  SelectImplRecursive(element, tokens, token_pos, options);
}

/**
 * add nodes satisfying the given tokens to result set.
 *
 * find children of this node which satisfies
 * tokens[token_pos:-1]. find children C1 of this node (or this
 * node itself) satisfying tokens[token_pos], then find C2
 * satisfying tokens[token_pos + 1] in all children of C1
 * recursively. finally when a node satisfying the last token (tokens[-1])
 * is found (which must have a parent/grandparent satisfying tokens[-2]
 * etc.), push it to result.
 */
void FiberElementSelector::SelectImplRecursive(
    FiberElement* element, const std::vector<SelectElementToken>& tokens,
    size_t token_pos, const SelectImplOptions& options) {
  // if we already have a result with first_only turned on, return!
  if (options.first_only && !result_.empty()) {
    return;
  }

  const SelectElementToken& token = tokens[token_pos];

  // if it is the target, add the node to the result
  bool token_satisfied = IsTokenSatisfied(element, token);
  bool is_last_token =
      token.combinator_to_next == SelectElementToken::Combinator::LAST;
  bool is_component = element->is_component();
  bool component_only_satisfied = !(options.component_only && !is_component);
  bool is_target_node =
      token_satisfied && is_last_token && component_only_satisfied;
  if (is_target_node) {
    InsertResult(element);
    if (options.first_only) {
      return;
    }
  }

  // search in children
  if (element->children().empty()) {
    return;
  }

  // two turns:
  // first turn (runs only if current token is satisfied), search in children
  // by next token;
  // second turn (if search in descendant allowed), search in children by
  // current token.
  base::InlineVector<size_t, 2> pos_to_check;
  if (token_satisfied && !is_last_token) {
    pos_to_check.push_back(token_pos + 1);
  }
  if (!options.no_descendant) {
    pos_to_check.push_back(token_pos);
  }

  for (size_t pos : pos_to_check) {
    auto next_options =
        PrepareNextSelectOptions(token, options, token_pos, pos);

    bool only_search_slots =
        (is_component && next_options.only_current_component) &&
        !options.is_root_component;
    if (only_search_slots) {
      if (next_options.parent_component_id.empty()) {
        next_options.parent_component_id = element->ParentComponentIdString();
      }
      SelectInSlots(element, tokens, pos, next_options,
                    next_options.parent_component_id);
    } else {
      // search in all children
      for (const auto& c : element->children()) {
        SelectImpl(c.get(), tokens, pos, next_options);
      }
    }
  }
}

bool FiberElementSelector::IsTokenSatisfied(FiberElement* node,
                                            const SelectElementToken& token) {
  switch (token.type) {
    case SelectElementToken::Type::CSS_SELECTOR:
      if (node->data_model()) {
        return node->data_model()->ContainsSelector(token.selector_string);
      }
      return false;
    case SelectElementToken::Type::REF_ID: {
      if (!node->data_model()) {
        return false;
      }
      const auto& attrs = node->data_model()->attributes();
      BASE_STATIC_STRING_DECL(kReactRef, "react-ref");
      auto it = attrs.find(kReactRef);
      return (it != attrs.end() &&
              it->second.StdString() == token.selector_string);
    }
    case SelectElementToken::Type::ELEMENT_ID: {
      int id;
      base::StringToInt(token.selector_string, &id, 10);
      return node->impl_id() == id;
    }
  }
}

void FiberElementSelector::SelectByElementId(SelectorItem* root,
                                             const NodeSelectOptions& options) {
  int id;
  if (!base::StringToInt(options.identifier, &id, 10)) {
    return;
  }

  auto element =
      static_cast<FiberElement*>(root)->element_manager()->node_manager()->Get(
          id);
  if (element) {
    InsertResult(static_cast<FiberElement*>(element));
  }
}

void FiberElementSelector::InsertResult(SelectorItem* base) {
  result_.push_back(static_cast<FiberElement*>(base));
}

bool FiberElementSelector::FoundElement() { return !result_.empty(); }

void FiberElementSelector::SelectInSlots(
    FiberElement* element, const std::vector<SelectElementToken>& tokens,
    size_t token_pos, const SelectImplOptions& options,
    const std::string& parent_component_id) {
  for (const auto& child : element->children()) {
    if (child->ParentComponentIdString() == parent_component_id) {
      SelectImpl(child.get(), tokens, token_pos, options);
    } else {
      SelectInSlots(child.get(), tokens, token_pos, options,
                    parent_component_id);
    }
  }
}

void FiberElementSelector::UniqueAndSortResult(FiberElement* root) {
  if (result_.size() < 2) {
    return;
  }

  std::unordered_map<FiberElement*, std::vector<size_t>> index_map;
  auto indexes = [&index_map](FiberElement* root, FiberElement* node) {
    if (index_map.count(node)) {
      return index_map[node];
    }
    std::vector<size_t> indexes;
    for (auto n = node; n != root;
         n = static_cast<FiberElement*>(n->parent())) {
      indexes.push_back(static_cast<FiberElement*>(n->parent())->IndexOf(n));
    }
    std::reverse(indexes.begin(), indexes.end());
    index_map[node] = indexes;
    return indexes;
  };

  std::sort(result_.begin(), result_.end(),
            [&indexes, root](FiberElement* a, FiberElement* b) {
              auto a_indexes = indexes(root, a);
              auto b_indexes = indexes(root, b);
              return a_indexes < b_indexes;
            });

  auto unique_iter = std::unique(result_.begin(), result_.end());
  result_.erase(unique_iter, result_.end());
}

}  // namespace tasm
}  // namespace lynx
