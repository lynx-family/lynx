// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_slot.h"

#include <list>
#include <utility>

#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace tasm {

RadonSlot::RadonSlot(const base::String& slot_name)
    : RadonBase(kRadonSlot, base::String(), kRadonInvalidNodeIndex),
      name_(slot_name) {}

RadonSlot::RadonSlot(const RadonSlot& node, PtrLookupMap& map)
    : RadonBase{node, map}, name_{node.name_} {}

void RadonSlot::MovePlugToComponent() {
  if (radon_children_.empty() || !radon_children_.back() || !radon_component_) {
    return;
  }
  // save original plugs into its component
  radon_children_.back()->radon_parent_ = nullptr;
  radon_component_->plugs()[name_] = std::move(radon_children_.back());
  radon_children_.clear();

  // Disconnect plug element from the plug's parent element.
  // Will reconnect later in ReFillSlotsAfterChildrenDiff if needed.
  radon_component_->radon_slots_helper()->DisconnectPlugElementFromParent(
      radon_component_->plugs()[name_].get());
}

void RadonSlot::WillRemoveNode() {
  if (!IsPlugCanBeMoved()) {
    RadonBase::WillRemoveNode();
    return;
  }
  if (will_remove_node_has_been_called_) {
    return;
  }
  will_remove_node_has_been_called_ = true;
  plug_can_be_moved_ = false;
  // save original plug to the plug's component
  MovePlugToComponent();
}

bool RadonSlot::CanBeReusedBy(const RadonBase* const radon_base) const {
  if (!RadonBase::CanBeReusedBy(radon_base)) {
    return false;
  }
  // Two slot can be reused only if their names are equal.
  const RadonSlot* const slot = static_cast<const RadonSlot* const>(radon_base);
  return name_.IsEqual(slot->name_);
}

void RadonSlot::RadonDiffChildren(
    const std::unique_ptr<RadonBase>& old_radon_child,
    const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonSlot::RadonDiffChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto old_radon_slot = static_cast<RadonSlot*>(old_radon_child.get());

  if (!old_radon_slot->IsPlugCanBeMoved()) {
    RadonBase::RadonDiffChildren(old_radon_child, option);
    return;
  }

  if (old_radon_slot->radon_children_.empty() || !radon_children_.empty()) {
    LOGE(
        "slot's children is not empty or old slot doesn't has children in "
        "RadonSlot::RadonDiffChildren.");
    return;
  }
  // just move the plug to new slot, the element structure has already been
  // setted.
  AddChild(std::move(old_radon_slot->radon_children_.back()));
}

void RadonSlot::SetComponent(RadonComponent* c) {
  RadonBase::SetComponent(c);
  // note: component in children of slot is different from slot, which is
  // already set by RadonPlug in AdoptPlug

  // register slot to it's component
  OnAttachToComponent();
}

void RadonSlot::AdoptPlug(std::unique_ptr<RadonBase> plug) {
  // in multi-layer slot, one slot may adopt the same plug twice.
  // issue: #4994
  if (radon_children_.size() != 0) {
    LOGE("RadonSlot isn't empty when adopt RadonPlug.");
    radon_children_.clear();
  }
  // node struct is different from virtual slot, plug itself is also added
  RadonComponent* comp = plug->component();
  RadonBase::AddChild(std::move(plug));
  radon_children_.back()->SetComponent(comp);
}

void RadonSlot::ReleasePlug() {
  // if current slot has child, should remove it's child first
  if (!radon_children_.empty()) {
    RadonBase* plug = radon_children_.front().get();
    RadonBase* plug_root =
        radon_children_.front()->radon_children_.front().get();
    plug_root->RemoveElementFromParent();
    RemoveChild(plug);
  }
}

void RadonSlot::AddChild(std::unique_ptr<RadonBase> child) {
  AdoptPlug(std::move(child));
}

void RadonSlot::AddSubTree(std::unique_ptr<RadonBase> child) {
  AdoptPlug(std::move(child));
}

void RadonSlot::OnAttachToComponent() {
  if (radon_component_) {
    radon_component_->AddRadonSlot(name_, this);
  }
}

void RadonSlot::ModifySubTreeComponent(RadonComponent* const target) {
  // iteratively set this and this's children's radon_component_ to target
  if (!target) {
    return;
  }
  SetComponent(target);
  if (!radon_children_.empty()) {
    // modify the plug's radon_component_
    radon_children_.front()->ModifySubTreeComponent(target->radon_component_);
  }
  return;
}

RadonPlug::RadonPlug(const base::String& plug_name, RadonComponent* component)
    : RadonBase(kRadonPlug, base::String(), kRadonInvalidNodeIndex),
      plug_name_(plug_name) {
  if (component) {
    radon_component_ = component->component();
  }
}

void RadonPlug::SetAttachedComponent(RadonComponent* component) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonPlug::SetAttachedComponent");
  if (!component) {
    return;
  }
  NeedModifySubTreeComponent(component->component());
}

RadonPlug::RadonPlug(const RadonPlug& plug, PtrLookupMap& map)
    : RadonBase{plug, map} {
  if (map.find(plug.radon_component_) != map.end()) {
    // should reset new created copy-plug's radon_component_ to new created
    // copy-component #issue: 3955
    SetComponent(static_cast<RadonComponent*>(map[plug.radon_component_]));
  }
}

void RadonPlug::WillRemoveNode() {
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

void RadonSlotsHelper::DisconnectPlugElementFromParent(RadonBase* plug) {
  std::list<RadonBase*> radon_base_list;

  for (auto& radon_base_child : plug->radon_children_) {
    radon_base_list.push_back(radon_base_child.get());
  }

  while (!radon_base_list.empty()) {
    RadonBase* front = radon_base_list.front();
    radon_base_list.pop_front();
    if (!front) {
      LOGE(
          "radon base is nullptr in "
          "RadonSlotsHelper::DisconnectPlugElementFromParent.");
      continue;
    }
    if (!front->NeedsElement()) {
      for (auto& radon_base_child : front->radon_children_) {
        radon_base_list.push_back(radon_base_child.get());
      }
      continue;
    }
    auto element = front->GetElementRef();
    if (element.get() != nullptr && element->parent()) {
      auto* parent = element->parent();
      parent->RemoveNode(element);
    }
  }
  return;
}

RadonSlotsHelper::RadonSlotsHelper(RadonComponent* radon_component)
    : radon_component_(radon_component) {}

void RadonSlotsHelper::MovePlugsFromSlots(NameToPlugMap& plugs,
                                          NameToSlotMap& slots) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonSlotsHelper::MovePlugsFromSlots");
  for (auto& slot : slots) {
    if (!slot.second->radon_children_.empty()) {
      // here we assume a plug is the child and only child of the related slot
      slot.second->radon_children_.back()->radon_parent_ = nullptr;
      plugs[slot.first] = std::move(slot.second->radon_children_.back());
    }
    slot.second->radon_children_.clear();
  }
}

void RadonSlotsHelper::DiffWithPlugs(NameToPlugMap& old_plugs,
                                     const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonSlotsHelper::DiffWithPlugs");
  auto& new_slots = radon_component_->slots();
  for (const auto& slot : new_slots) {
    auto& new_slot_ptr = slot.second;
    auto& name = slot.first;
    auto it = old_plugs.find(name);
    if (it == old_plugs.end() || it->second == nullptr) {
      // case 1: old plug is not found, dispatch new plug
      // also need to ensure new plug exists
      if (!new_slot_ptr->radon_children_.empty() &&
          new_slot_ptr->radon_children_.back()) {
        new_slot_ptr->radon_children_.back()->DispatchForDiff(option);
      }
      continue;
    }
    auto& old_plug_ptr = it->second;
    if (!new_slot_ptr->radon_children_.empty() &&
        new_slot_ptr->radon_children_.back()) {
      // case 2: both the new plug and the old plug exist
      // diff new plug vs old plug
      new_slot_ptr->radon_children_.back()->RadonDiffChildren(old_plug_ptr,
                                                              option);
    } else {
      // case 3: new plug is not found, but old plug exists
      // we need to delete old plug
      RadonBase* plug_root = old_plug_ptr->radon_children_.front().get();
      plug_root->RemoveElementFromParent();
      plug_root->ClearChildrenRecursivelyInPostOrder();
    }
  }
}

void RadonSlotsHelper::ReFillSlotsAfterChildrenDiff(
    NameToSlotMap& old_slots, const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "RadonSlotsHelper::ReFillSlotsAfterChildrenDiff");
  auto& new_slots = radon_component_->slots();
  auto& new_plugs = radon_component_->plugs();
  // the old plug has already been attached to the new slot,
  // but if the class_transmit logic or the css variable logic has been changed,
  // still need to re-dispatch the plug.
  if (!option.class_transmit_.IsEmpty() || option.css_variable_changed_ ||
      option.global_properties_changed_) {
    for (auto& slot : new_slots) {
      if (!slot.second) {
        continue;
      }
      if (slot.second->radon_children_.size() == 0) {
        LOGE(
            "The new slot doesn't has related plug in "
            "ReFillSlotsAfterChildrenDiff");
        continue;
      }
      RadonPlug* plug_to_reattach =
          static_cast<RadonPlug*>(slot.second->radon_children_.front().get());
      if (plug_to_reattach) {
        plug_to_reattach->SetAttachedComponent(radon_component_);
        // when plug's element reset and Redispatch
        // should notify devtool to delete component stored plug pointer
        plug_to_reattach->RemoveElementFromParent();
        plug_to_reattach->ResetElementRecursively();
        plug_to_reattach->DispatchForDiff(option);
      }
    }
  }
  for (auto& plug : new_plugs) {
    if (!plug.second) {
      continue;
    }
    auto slot_iter = new_slots.find(plug.first);
    // case 1: the slot exists only in old component,
    // just remove the related element
    // Notice: here we shouldn't delete the plug because we may reuse the plug's
    // information and re-dispatch this plug later.
    if (slot_iter == new_slots.end()) {
      if (old_slots.find(plug.first) != old_slots.end()) {
        new_plugs[plug.first]->RemoveElementFromParent();
        // handle lifecycle.
        new_plugs[plug.first]->OnComponentRemovedInPostOrder();
        new_plugs[plug.first]->ResetElementRecursively();
      }
      continue;
    }
    radon_component_->AddRadonPlug(plug.first, std::move(plug.second));
    // case 2: the slot exists only in new component or the slot need to be
    // updated because of transmit-class, should delete element and re-dispatch
    // the plug. Why need to ResetElementRecursively and dispatch this plug like
    // a new created node? Because the plug's related element may has been
    // removed
    if (new_slots[plug.first]->radon_children_.size() == 0) {
      LOGE(
          "The new slot doesn't has related plug in "
          "ReFillSlotsAfterChildrenDiff");
      continue;
    }
    RadonPlug* plug_to_reattach = static_cast<RadonPlug*>(
        new_slots[plug.first]->radon_children_.front().get());
    if (plug_to_reattach && plug_to_reattach->IsConnectedWithRootNode()) {
      plug_to_reattach->SetAttachedComponent(radon_component_);
      plug_to_reattach->ResetElementRecursively();
      plug_to_reattach->DispatchForDiff(option);
    }
  }
}

void RadonSlotsHelper::FillUnattachedPlugs() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonSlotsHelper::FillUnattachedPlugs");
  auto& plugs = radon_component_->plugs();
  for (auto& plug : plugs) {
    radon_component_->AddRadonPlug(plug.first, std::move(plug.second));
  }
}

void RadonSlotsHelper::RemoveAllSlots() { radon_component_->slots().clear(); }

}  // namespace tasm
}  // namespace lynx
