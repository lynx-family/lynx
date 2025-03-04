// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/block_element.h"

namespace lynx {
namespace tasm {

namespace {
constexpr static int32_t kInvalidIndex = -1;
}

void BlockElement::InsertNode(const fml::RefPtr<Element> &raw_child) {
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);

  if (parent_ && parent_->is_fiber_element()) {
    child->set_virtual_parent(this);
    size_t index = FindInsertIndex(child);
    if (index == static_cast<FiberElement *>(parent_)->children().size()) {
      // equal to size: append this node to the end
      static_cast<FiberElement *>(parent_)->InsertNode(child);
    } else {
      static_cast<FiberElement *>(parent_)->InsertNode(child,
                                                       static_cast<int>(index));
    }
  }
  size_t index = FindBlockInsertIndex(child);
  AddBlockChildAt(child, index);
}

void BlockElement::RemoveNode(const fml::RefPtr<Element> &raw_child,
                              bool destroy) {
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);

  if (child->is_block()) {
    static_cast<BlockElement *>(child.get())->RemoveAllBlockNodes();
  } else if (parent_ && parent_->is_fiber_element()) {
    static_cast<FiberElement *>(parent_)->RemoveNode(child);
  }
  child->set_virtual_parent(nullptr);
  size_t index = IndexOfBlockChild(child);
  RemoveBlockChildAt(index);
}

void BlockElement::RemoveAllBlockNodes() {
  if (parent_ && block_children_.size() > 0) {
    for (int index = static_cast<int>(block_children_.size()) - 1; index >= 0;
         --index) {
      RemoveNode(block_children_[index]);
    }
  }
}

size_t BlockElement::FindInsertIndex(const fml::RefPtr<FiberElement> &child) {
  size_t offset = 0;
  FiberElement *virtual_parent = this->virtual_parent();
  FiberElement *current = this;

  // If there are multiple virtual nodes from child to the parent node, it is
  // necessary to find the offset under each virtual node and obtain the total
  // offset under the root virtual parent.
  while (virtual_parent != nullptr) {
    BlockElement *block_element = static_cast<BlockElement *>(virtual_parent);
    size_t idx = 0;
    for (auto iter = block_element->block_children_.rbegin();
         iter != block_element->block_children_.rend(); ++iter) {
      if (current->impl_id() < (*iter)->impl_id()) {
        if ((*iter)->is_block()) {
          idx += static_cast<BlockElement *>((*iter).get())
                     ->GetAllNodeCountExcludeBlock();
        } else {
          idx++;
        }
      } else {
        break;
      }
    }
    current = virtual_parent;
    virtual_parent = virtual_parent->virtual_parent();
    offset = offset + idx;
  }

  const auto &children = static_cast<FiberElement *>(parent_)->children();
  size_t index = children.size();
  // Find the order of root virtual parent in parent node due to the impl_id is
  // arranged from small to large.
  for (auto iter = children.rbegin(); iter != children.rend(); ++iter) {
    int32_t impl_id = (*iter)->impl_id();
    if ((*iter)->root_virtual_parent() != nullptr) {
      impl_id = (*iter)->root_virtual_parent()->impl_id();
    }
    if (child->root_virtual_parent()->impl_id() < impl_id) {
      index--;
    } else {
      break;
    }
  }
  index = index - offset;
  return index;
}

size_t BlockElement::GetAllNodeCountExcludeBlock() {
  size_t count = 0;
  for (auto iter = block_children_.begin(); iter != block_children_.end();
       ++iter) {
    if ((*iter)->is_block()) {
      count += static_cast<BlockElement *>((*iter).get())
                   ->GetAllNodeCountExcludeBlock();
    } else {
      count++;
    }
  }
  return count;
}

void BlockElement::AddBlockChildAt(const fml::RefPtr<FiberElement> &child,
                                   size_t index) {
  block_children_.insert(block_children_.begin() + index, child);
}

size_t BlockElement::FindBlockInsertIndex(
    const fml::RefPtr<FiberElement> &child) {
  size_t index = block_children_.size();
  for (auto iter = block_children_.rbegin(); iter != block_children_.rend();
       ++iter) {
    // The impl id of elements in vector are arranged from small to large. Find
    // the first element whose lepus id is greater than the element to be
    // inserted.
    if (child->impl_id() < (*iter)->impl_id()) {
      index--;
    } else {
      break;
    }
  }
  return index;
}

void BlockElement::RemoveBlockChildAt(size_t index) {
  if (index >= 0 && index < block_children_.size()) {
    block_children_.erase(block_children_.begin() + index);
  }
}

size_t BlockElement::IndexOfBlockChild(const fml::RefPtr<FiberElement> &child) {
  for (size_t index = 0; index < block_children_.size(); ++index) {
    if (block_children_[index]->impl_id() == child->impl_id()) {
      return index;
    }
  }
  return kInvalidIndex;
}

}  // namespace tasm
}  // namespace lynx
