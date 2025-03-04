// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_factory.h"

#include <memory>
#include <utility>

#include "core/renderer/dom/vdom/radon/radon_base.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"
#include "core/renderer/dom/vdom/radon/radon_list_base.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/dom/vdom/radon/radon_types.h"
namespace lynx {
namespace tasm {
namespace radon_factory {
namespace {
std::unique_ptr<RadonBase> MakeRadon(const RadonBase& node, PtrLookupMap& map) {
  auto copy = std::unique_ptr<RadonBase>{nullptr};
  switch (node.NodeType()) {
    case kRadonNode:
      copy =
          std::make_unique<RadonNode>(static_cast<const RadonNode&>(node), map);
      break;
    case kRadonBlock:
      copy = std::make_unique<RadonBase>(node, map);
      break;
    case kRadonListNode:
      copy = std::make_unique<RadonListBase>(
          static_cast<const RadonListBase&>(node), map);
      break;
    case kRadonSlot:
      copy =
          std::make_unique<RadonSlot>(static_cast<const RadonSlot&>(node), map);
      break;
    case kRadonPlug:
      copy =
          std::make_unique<RadonPlug>(static_cast<const RadonPlug&>(node), map);
      break;
    case kRadonComponent:
      copy = std::make_unique<RadonComponent>(
          static_cast<const RadonComponent&>(node), map);
      break;
    case kRadonLazyComponent:
      copy = std::make_unique<RadonLazyComponent>(
          static_cast<const RadonLazyComponent&>(node), map);
      break;
    case kRadonPage:
    case kRadonUnknown:
      LOGF("radon_factory called with uncopyable RadonBase");
  }
  return copy;
}

void CopyAndAddToParent(const RadonBase& node, RadonBase& parent,
                        PtrLookupMap& map) {
  auto copy = MakeRadon(node, map);
  auto& copyNode = *copy;
  // map the original node to its copy
  map[&const_cast<RadonBase&>(node)] = copy.get();
  // transfer the ownership of the copy to its parent
  // this must be done in a preorder for that :
  // - AddChild set the component for this node.
  // - All nodes need to know their component (or the node itself is a
  // component)
  //   to set their children's component.
  parent.AddChild(std::move(copy));

  for (const auto& child : node.radon_children_) {
    CopyAndAddToParent(*child, copyNode, map);
  }
}
}  // namespace

std::unique_ptr<RadonBase> Copy(const RadonBase& node, PtrLookupMap& map) {
  auto copy = MakeRadon(node, map);
  map[&const_cast<RadonBase&>(node)] = copy.get();

  for (const auto& child : node.radon_children_) {
    CopyAndAddToParent(*child, *copy, map);
  }

  // transfer the ownership of the copy to the caller.
  return copy;
}

std::unique_ptr<RadonBase> Copy(const RadonBase& node) {
  auto map = PtrLookupMap{};
  return Copy(node, map);
}

RadonBase* CopyRadonRawPtrForDiff(RadonBase& node, PtrLookupMap& map) {
  RadonBase* copy = nullptr;
  switch (node.NodeType()) {
    case kRadonNode:
      copy = new RadonNode(static_cast<const RadonNode&>(node), map);
      break;
    case kRadonBlock:
      copy = new RadonBase(node, map);
      break;
    case kRadonListNode:
      copy = new RadonListBase(static_cast<const RadonListBase&>(node), map);
      break;
    case kRadonSlot:
      copy = new RadonSlot(static_cast<const RadonSlot&>(node), map);
      break;
    case kRadonPlug:
      copy = new RadonPlug(static_cast<const RadonPlug&>(node), map);
      break;
    case kRadonComponent:
      copy = new RadonComponent(static_cast<const RadonComponent&>(node), map);
      break;
    case kRadonLazyComponent:
      copy = new RadonLazyComponent(
          static_cast<const RadonLazyComponent&>(node), map);
      break;
    case kRadonPage:
    case kRadonUnknown:
      LOGE("radon_factory called with uncopyable RadonBase");
  }
  return copy;
}

void CopyRadonDiffSubTreeAndAddToParent(RadonBase& parent, RadonBase& node,
                                        PtrLookupMap& map) {
  auto* self = CopyRadonRawPtrForDiff(node, map);
  if (!self) {
    return;
  }
  parent.AddChild(std::unique_ptr<RadonBase>(self));
  for (auto& child : node.radon_children_) {
    CopyRadonDiffSubTreeAndAddToParent(*self, *child, map);
  }
}

RadonBase* CopyRadonDiffSubTree(RadonBase& node) {
  auto map = PtrLookupMap{};
  auto* self = CopyRadonRawPtrForDiff(node, map);
  if (!self) {
    return nullptr;
  }
  for (auto& child : node.radon_children_) {
    CopyRadonDiffSubTreeAndAddToParent(*self, *child, map);
  }
  return self;
}

}  // namespace radon_factory
}  // namespace tasm
}  // namespace lynx
