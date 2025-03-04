// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/node_selector.h"

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>

#include "base/include/string/string_number_convert.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/select_element_token.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {
namespace tasm {

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
void RadonNodeSelector::SelectImpl(
    SelectorItem* element_base, const std::vector<SelectElementToken>& tokens,
    size_t token_pos, const SelectImplOptions& options) {
  // if we already have a result with first_only turned on, return!
  if (options.first_only && !result_.empty()) {
    return;
  }
  const SelectElementToken& token = tokens[token_pos];

  RadonBase* base = static_cast<RadonBase*>(element_base);

  // add the node to the result if it is the target
  bool token_satisfied = IsTokenSatisfied(base, token);
  bool is_last_token =
      token.combinator_to_next == SelectElementToken::Combinator::LAST;
  bool is_component = base->IsRadonComponent() && !base->IsRadonPage();
  bool component_only_satisfied = !(options.component_only && !is_component);
  bool is_target_node =
      token_satisfied && is_last_token && component_only_satisfied;
  if (is_target_node) {
    InsertResult(base);
    if (options.first_only) {
      return;
    }
  }

  // search in children
  if (base->radon_children_.empty()) {
    return;
  }

  // two turns:
  // first turn (runs only if current token is satisfied), search in children
  // by next token;
  // second turn (if search in descendant allowed), search in children by
  // current token.
  std::vector<size_t> pos_to_check;
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
      SelectInSlots(static_cast<RadonComponent*>(base), tokens, pos,
                    next_options);
    } else {
      // search in all children
      for (const auto& c : base->radon_children_) {
        SelectImpl(c.get(), tokens, pos, next_options);
      }
    }
  }
}

void RadonNodeSelector::SelectInSlots(
    RadonComponent* component, const std::vector<SelectElementToken>& tokens,
    size_t pos, const SelectImplOptions& options) {
  const auto& slots = component->slots();
  for (const auto& [name, slot] : slots) {
    if (!slot) {
      continue;
    }
    for (const auto& plug : slot->radon_children_) {
      if (!plug) {
        continue;
      }
      for (const auto& plug_content : plug->radon_children_) {
        if (!plug_content) {
          continue;
        }
        // when tt:if set to false, corresponding slot could still be in
        // slots. we shouldn't search in it in that case. check
        // whether it's really on the tree.
        const bool is_on_tree =
            plug_content->component() == component->component();
        if (is_on_tree) {
          SelectImpl(plug_content.get(), tokens, pos, options);
        }
      }
    }
  }
}

bool RadonNodeSelector::IsTokenSatisfied(RadonBase* base,
                                         const SelectElementToken& token) {
  if (!base->IsRadonNode()) {
    return false;
  }
  RadonNode* node = static_cast<RadonNode*>(base);
  switch (token.type) {
    case SelectElementToken::Type::CSS_SELECTOR:
      return node->ContainsSelector(token.selector_string);
    case SelectElementToken::Type::REF_ID: {
      BASE_STATIC_STRING_DECL(kReactRef, "react-ref");
      auto iter = node->attributes().find(kReactRef);
      return iter != node->attributes().end() &&
             iter->second.StdString() == token.selector_string;
    }
    case SelectElementToken::Type::ELEMENT_ID: {
      int id;
      base::StringToInt(token.selector_string, &id, 10);
      return node->ImplId() == id;
    }
  }
}

RadonNodeSelectResult RadonNodeSelector::Select(
    RadonBase* root, const NodeSelectOptions& options) {
  if (root == nullptr) {
    RadonNodeSelectResult result{{}, options};
    result.root_found = false;
    return result;
  }
  LOGI(" SelectNode: " << options.ToString()
                       << ", root_impl_id: " << root->ImplId());
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonNodeSelector::Select");

  RadonNodeSelector selector;
  selector.Distribute(root, options);
  selector.UniqueAndSortResult(root);
  return RadonNodeSelectResult{std::move(selector.result_), options,
                               selector.identifier_legal_};
}

RadonNodeSelectResult RadonNodeSelector::Select(
    RadonPage* page, const NodeSelectRoot& root,
    const NodeSelectOptions& options) {
  LOGI(" SelectNodeRoot: " << root.ToPrettyString());
  RadonBase* base = nullptr;
  switch (root.root_type) {
    case NodeSelectRoot::RootType::COMPONENT_ID:
      base = page->GetComponent(root.component_id);
      break;
    case NodeSelectRoot::RootType::NODE_UNIQUE_ID: {
      if (page->proxy_) {
        auto element = page->proxy_->element_manager()->node_manager()->Get(
            root.node_unique_id);
        if (element) {
          base = element->data_model()->radon_node_ptr();
        }
      }
    } break;
  }
  return Select(base, options);
}

void RadonNodeSelector::SelectByElementId(SelectorItem* root,
                                          const NodeSelectOptions& options) {
  int id;

  if (static_cast<RadonBase*>(root)->component() == nullptr ||
      !base::StringToInt(options.identifier, &id, 10)) {
    return;
  }

  auto element = static_cast<RadonBase*>(root)
                     ->component()
                     ->page_proxy_->element_manager()
                     ->node_manager()
                     ->Get(id);
  if (element) {
    InsertResult(element->data_model()->radon_node_ptr());
  }
}

void RadonNodeSelector::InsertResult(SelectorItem* element_base) {
  if (element_base) {
    result_.push_back(static_cast<RadonNode*>(element_base));
  }
}

bool RadonNodeSelector::FoundElement() { return !result_.empty(); }

void RadonNodeSelector::UniqueAndSortResult(RadonBase* root) {
  if (result_.size() < 2) {
    return;
  }

  std::unordered_map<RadonBase*, std::vector<size_t>> index_map;
  auto indexes = [&index_map](RadonBase* root, RadonBase* node) {
    if (index_map.count(node)) {
      return index_map[node];
    }
    std::vector<size_t> indexes;
    for (auto n = node; n != root; n = n->Parent()) {
      indexes.push_back(n->IndexInSiblings());
    }
    std::reverse(indexes.begin(), indexes.end());
    index_map[node] = indexes;
    return indexes;
  };

  std::sort(result_.begin(), result_.end(),
            [&indexes, root](RadonNode* a, RadonNode* b) {
              auto a_indexes = indexes(root, a);
              auto b_indexes = indexes(root, b);
              return a_indexes < b_indexes;
            });

  auto unique_iter = std::unique(result_.begin(), result_.end());
  result_.erase(unique_iter, result_.end());
}

}  // namespace tasm
}  // namespace lynx
