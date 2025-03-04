// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_base.h"

#include <sstream>
#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/select_element_token.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

constexpr const static char* kDefaultPageTag = "page";

RadonBase::RadonBase(RadonNodeType node_type, const base::String& tag_name,
                     RadonNodeIndexType node_index)
    : node_type_{node_type}, node_index_{node_index}, tag_name_{tag_name} {}

RadonBase::RadonBase(const RadonBase& node, PtrLookupMap& map)
    : radon_component_{node.radon_component_},
      node_type_{node.node_type_},
      node_index_{node.node_index_},
      tag_name_{node.tag_name_} {}

void RadonBase::AddChild(std::unique_ptr<RadonBase> child) {
  child->SetComponent(radon_component_);
  AddChildWithoutSetComponent(std::move(child));
}

void RadonBase::AddChildWithoutSetComponent(std::unique_ptr<RadonBase> child) {
  child->radon_parent_ = this;
  child->radon_previous_ = LastChild();
  if (!radon_children_.empty()) {
    LastChild()->radon_next_ = child.get();
  }
  radon_children_.push_back(std::move(child));
}

void RadonBase::AddSubTree(std::unique_ptr<RadonBase> child) {
  AddChildWithoutSetComponent(std::move(child));
}

std::unique_ptr<RadonBase> RadonBase::RemoveChild(RadonBase* child) {
  auto it = find_if(radon_children_.begin(), radon_children_.end(),
                    [child](std::unique_ptr<RadonBase>& pChild) {
                      return pChild.get() == child;
                    });
  if (it == radon_children_.end()) {
    return std::unique_ptr<RadonBase>(nullptr);
  }
  if (child->radon_previous_) {
    child->radon_previous_->radon_next_ = child->radon_next_;
  }
  if (child->radon_next_) {
    child->radon_next_->radon_previous_ = child->radon_previous_;
  }
  auto deleted_child = std::move(*it);
  radon_children_.erase(it);
  return deleted_child;
}

void RadonBase::ClearChildrenRecursivelyInPostOrder() {
  for (auto& child : radon_children_) {
    if (child) {
      child->ClearChildrenRecursivelyInPostOrder();
    }
  }
  radon_children_.clear();
}

void RadonBase::OnComponentRemovedInPostOrder() {
  for (auto& child : radon_children_) {
    if (child) {
      child->OnComponentRemovedInPostOrder();
    }
  }
}

void RadonBase::SetComponent(RadonComponent* component) {
  radon_component_ = component;
}

void RadonBase::Dispatch(const DispatchOption& option) {
  DispatchSelf(option);
  DispatchSubTree(option);
}

void RadonBase::DispatchSelf(const DispatchOption& option) {}

void RadonBase::DispatchSubTree(const DispatchOption& option) {
  EXEC_EXPR_FOR_INSPECTOR(
      DispatchOptionObserverForInspector observer(option, this));
  if (dispatched_ && option.class_transmit_.IsEmpty() &&
      !option.css_variable_changed_ && !option.global_properties_changed_ &&
      !option.ssr_hydrating_) {
    // TBD(zhangkaijie.9): remove this RADON_ONLLY branch.
  } else {
    DispatchChildren(option);
  }
  dispatched_ = true;
}

void RadonBase::DispatchChildren(const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DispatchChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  for (auto& child : radon_children_) {
    if (child) {
      child->Dispatch(option);
    }
  }
}

void RadonBase::DispatchForDiff(const DispatchOption& option) {
  DispatchSelf(option);
  DispatchChildrenForDiff(option);
  dispatched_ = true;
}

void RadonBase::DispatchChildrenForDiff(const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DispatchChildrenForDiff",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  EXEC_EXPR_FOR_INSPECTOR(
      DispatchOptionObserverForInspector observer(option, this));
  for (auto& child : radon_children_) {
    if (child) {
      child->DispatchForDiff(option);
    }
  }
}

// Radon Element Structure

void RadonBase::ResetElementRecursively() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonBase::ResetElementRecursively",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  dispatched_ = false;
  for (auto& node : radon_children_) {
    if (node) {
      node->ResetElementRecursively();
    }
  }
}

void RadonBase::WillRemoveNode() {
  if (will_remove_node_has_been_called_) {
    return;
  }
  will_remove_node_has_been_called_ = true;
  for (auto& node : radon_children_) {
    if (node) {
      node->WillRemoveNode();
    }
  }
}

void RadonBase::RemoveElementFromParent() {
  for (auto& node : radon_children_) {
    if (node) {
      node->RemoveElementFromParent();
    }
  }
}

bool RadonBase::GetDevToolFlag() {
  RadonNode* root = root_node();
  return root && root->page_proxy_->element_manager()->GetDevToolFlag() &&
         root->page_proxy_->element_manager()->IsDomTreeEnabled();
}

Element* RadonBase::PreviousSiblingElement() {
  if (radon_previous_) {
    auto* element = radon_previous_->LastNoFixedElement();
    if (element) {
      return element;
    }
    return radon_previous_->PreviousSiblingElement();
  }
  // radon_previous == nullptr
  if (radon_parent_) {
    if (radon_parent_->NeedsElement()) {
      return nullptr;
    }
    return radon_parent_->PreviousSiblingElement();
  }
  return nullptr;
}

RadonElement* RadonBase::radon_element() const { return nullptr; }

const fml::RefPtr<Element>& RadonBase::GetElementRef() const {
  base::NoDestructor<fml::RefPtr<Element>> kNullElement{nullptr};
  return *kNullElement;
}

Element* RadonBase::LastNoFixedElement() const {
  if (NeedsElement()) {
    // issue: #4954
    // When the element is the first layer child of the root page, should just
    // return this element no matter it is fixed or not. Otherwise if the
    // element is fixed, we may insert next element in a wrong index.
    if (element() && radon_parent_->IsRadonPage()) {
      return element();
    }
    if (element() && !element()->is_fixed()) {
      return element();
    }
    return nullptr;
  }
  for (auto it = radon_children_.rbegin(); it != radon_children_.rend(); it++) {
    Element* element = nullptr;
    if ((*it)->NeedsElement()) {
      element = (*it)->element();
    } else {
      element = (*it)->LastNoFixedElement();
    }
    if (element != nullptr && !element->is_fixed()) {
      return element;
    }
  }
  return nullptr;
}

bool RadonBase::IsConnectedWithRootNode() {
  // TODO(kechenglong): later, root_node_ should be set to nullptr if and only
  // if when the node is disconnected.
  root_node_ = nullptr;
  // We reused root_node() function to check whether root_node_ is a nullptr.
  return root_node() != nullptr;
}

RadonPage* RadonBase::root_node() {
  if (root_node_ == nullptr) {
    RadonBase* node = this;
    while (node->Parent() != nullptr) {
      node = node->Parent();
    }
    if (node->IsRadonPage()) {
      root_node_ = static_cast<RadonPage*>(node);
    }
  }
  return root_node_;
}

RadonComponent* RadonBase::GetRootEntryNode() {
  if (root_entry_node_ == nullptr) {
    RadonBase* node = Parent();
    if (node != nullptr) {
      while (!(node->IsRadonLazyComponent() || node->IsRadonPage())) {
        node = node->Parent();
        if (node == nullptr) {
          return nullptr;
        }
      }
      root_entry_node_ = static_cast<RadonComponent*>(node);
    }
  }
  return root_entry_node_;
}

Element* RadonBase::GetRootElement() {
  if (root_element_ == nullptr) {
    RadonPage* radon_page = root_node();
    if (!radon_page->page_proxy_->GetPageElementEnabled()) {
      root_element_ = radon_page->element();
    } else if (!radon_page->radon_children_.empty() &&
               radon_page->radon_children_.front()->tag_name_.IsEqual(
                   kDefaultPageTag)) {
      // if page_element enabled, the root_element should be the first child of
      // RadonPage.
      root_element_ = radon_page->radon_children_.front()->element();
    }
  }
  return root_element_;
}

Element* RadonBase::ParentElement() {
  auto* parent = radon_parent_;
  while (parent) {
    if (parent->NeedsElement()) {
      return parent->element();
    }
    parent = parent->radon_parent_;
  }
  return nullptr;
}

RadonBase* RadonBase::LastChild() {
  if (radon_children_.empty()) {
    return nullptr;
  } else {
    return radon_children_.back().get();
  }
}

void RadonBase::Visit(bool including_self,
                      const base::MoveOnlyClosure<bool, RadonBase*>& visitor) {
  bool visit_children = true;
  if (including_self) {
    visit_children = visitor(this);
  }
  if (!visit_children) return;
  for (auto& child : radon_children_) {
    child->Visit(true, visitor);
  }
}

bool RadonBase::SetLynxKey(const base::String& key, const lepus::Value& value) {
  if (key.IsEqual(kLynxKey)) {
    lynx_key_ = value;
    return true;
  }
  return false;
}

void RadonBase::RadonMyersDiff(RadonBaseVector& old_radon_children,
                               const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonBase::RadonMyersDiff",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto& new_radon_children = radon_children_;
  auto can_reuse_node = [](const std::unique_ptr<RadonBase>& lhs,
                           const std::unique_ptr<RadonBase>& rhs) {
    return lhs->CanBeReusedBy(rhs.get());
  };
  myers_diff::DiffResultBase actions;

  actions = myers_diff::MyersDiffWithoutUpdate(
      old_radon_children.begin(), old_radon_children.end(),
      new_radon_children.begin(), new_radon_children.end(), can_reuse_node);

  if (!(option.need_diff_ ||
        (actions.removals_.empty() && actions.insertions_.empty()))) {
    LYNX_ERROR(error::E_SSR_HYDRATE_DOM_DEVIATE_FROM_SSR_RESULT,
               "Dom structure deviates from SSR result after hydration.", "");
  }

  uint32_t old_index = 0, new_index = 0;
  uint32_t actions_removals_index = 0, actions_insertions_index = 0;

  if (actions.removals_.size() > 0 || actions.insertions_.size() > 0) {
    option.has_patched_ = true;
  }

  while (new_index < new_radon_children.size() ||
         old_index < old_radon_children.size()) {
    // remove radon node
    if (actions_removals_index < actions.removals_.size() &&
        static_cast<uint32_t>(actions.removals_[actions_removals_index]) ==
            old_index) {
      // here we just modify ElementTree, no need to modify RadonTree because
      // RadonTree would be modified correctly later.
      old_radon_children[old_index]->WillRemoveNode();
      old_radon_children[old_index]->RemoveElementFromParent();
      ++old_index;
      ++actions_removals_index;
      // insert radon node
    } else if (actions_insertions_index < actions.insertions_.size() &&
               static_cast<uint32_t>(
                   actions.insertions_[actions_insertions_index]) ==
                   new_index) {
      new_radon_children[new_index]->DispatchForDiff(option);
      ++new_index;
      ++actions_insertions_index;
      // diff radon node with same node_index
    } else if (new_index < new_radon_children.size() &&
               old_index < old_radon_children.size()) {
      DCHECK(new_radon_children[new_index]->node_index_ ==
             old_radon_children[old_index]->node_index_);
      auto& new_radon_child = new_radon_children[new_index];
      auto& old_radon_child = old_radon_children[old_index];
      new_radon_child->SwapElement(old_radon_child, option);
      new_radon_child->RadonDiffChildren(old_radon_child, option);
      ++new_index;
      ++old_index;
    } else {
      LOGF("RadonMyersDiff fatal.");
    }
  }
  if (!option.only_swap_element_) {
    // diff finished, handle old radon tree
    // just destruct the radon tree if this radon tree is not reusable.
    for (auto& old_child : old_radon_children) {
      old_child->WillRemoveNode();
    }
    for (auto& old_child : old_radon_children) {
      old_child->ClearChildrenRecursivelyInPostOrder();
    }
    old_radon_children.clear();
  }
}

void RadonBase::RadonDiffChildren(
    const std::unique_ptr<RadonBase>& old_radon_child,
    const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonBase::RadonDiffChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  RadonMyersDiff(old_radon_child->radon_children_, option);
}

void RadonBase::NeedModifySubTreeComponent(RadonComponent* const target) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonBase::NeedModifySubTreeComponent",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  ModifySubTreeComponent(target);
}

void RadonBase::ModifySubTreeComponent(RadonComponent* const target) {
  // iteratively set this and this's children's radon_component_ to target
  if (!target) {
    return;
  }
  radon_component_ = target;
  for (auto& child : radon_children_) {
    child->ModifySubTreeComponent(target);
  }
}

bool RadonBase::CanBeReusedBy(const RadonBase* const radon_base) const {
  return node_index_ == radon_base->node_index_ &&
         node_type_ == radon_base->node_type_ &&
         tag_name_ == radon_base->tag_name_ &&
         lynx_key_ == radon_base->lynx_key_;
}

int32_t RadonBase::IndexInSiblings() const {
  if (Parent() == nullptr) {
    return 0;
  }

  if (NodeType() == kRadonPlug) {
    return Parent()->IndexInSiblings();
  }
  if (Parent()->NodeType() == kRadonPlug) {
    auto slot = Parent()->Parent();
    return slot->IndexInSiblings();
  }

  auto& siblings = Parent()->radon_children_;
  auto iter =
      std::find_if(siblings.begin(), siblings.end(),
                   [id = ImplId()](auto& ptr) { return ptr->ImplId() == id; });
  return static_cast<int32_t>(std::distance(siblings.begin(), iter));
}

void RadonBase::triggerNewLifecycle(const DispatchOption& option) {
  for (auto& child : radon_children_) {
    child->triggerNewLifecycle(option);
  }
}

}  // namespace tasm
}  // namespace lynx
