// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/block_element.h"

#include <algorithm>
#include <unordered_set>

namespace lynx {
namespace tasm {

void BlockElement::InsertNode(const fml::RefPtr<Element> &raw_child) {
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);

  child->set_virtual_parent(this);
  if (parent_) {
    auto *parent = static_cast<FiberElement *>(parent_);
    if (child->is_block()) {
      child->set_parent(parent);
      parent->scoped_virtual_children_->push_back(child);
    } else {
      size_t index = FindInsertIndex(child);
      auto *ref_node =
          index < parent->children().size()
              ? static_cast<FiberElement *>(parent->GetChildAt(index))
              : nullptr;
      parent->InsertNodeBeforeInternal(child, ref_node, false);
    }
  }
  AddBlockChildAt(child, block_children_.size());
}

void BlockElement::RemoveNode(const fml::RefPtr<Element> &raw_child,
                              bool destroy) {
  auto child = fml::static_ref_ptr_cast<FiberElement>(raw_child);

  if (child->is_block()) {
    static_cast<BlockElement *>(child.get())->RemoveAllBlockNodes();
    // Remove this BlockElement from parent's scoped_virtual_children_
    if (parent_) {
      auto parent_fiber = static_cast<FiberElement *>(parent_);
      parent_fiber->RemoveLogicalChild(child);
      if (parent_fiber->scoped_virtual_children_.has_value()) {
        auto &virtual_children = *parent_fiber->scoped_virtual_children_;
        auto it = std::find_if(virtual_children.begin(), virtual_children.end(),
                               [&child](const fml::RefPtr<Element> &elem) {
                                 return elem.get() == child.get();
                               });
        if (it != virtual_children.end()) {
          virtual_children.erase(it);
        }
      }
    }
  } else if (parent_) {
    static_cast<FiberElement *>(parent_)->RemoveNodeInternal(child, destroy,
                                                             false);
  }
  child->set_virtual_parent(nullptr);
  child->set_parent(nullptr);
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
  size_t local_offset = 0;
  BlockElement *virtual_parent =
      static_cast<BlockElement *>(this->virtual_parent());
  BlockElement *current = this;

  // Calculate the prefix offset within the current block.
  for (const auto &block_child : block_children_) {
    if (block_child.get() == child.get()) {
      break;
    }
    if (block_child->is_block()) {
      local_offset += static_cast<BlockElement *>(block_child.get())
                          ->GetAllNodeCountExcludeBlock();
    } else {
      local_offset++;
    }
  }

  // Traverse up the virtual parent chain to accumulate preceding offsets.
  while (virtual_parent != nullptr) {
    size_t prefix = 0;

    for (const auto &sibling : virtual_parent->block_children()) {
      if (sibling.get() == current) {
        break;
      }
      if (sibling->is_block()) {
        prefix += static_cast<BlockElement *>(sibling.get())
                      ->GetAllNodeCountExcludeBlock();
      } else {
        prefix++;
      }
    }

    local_offset += prefix;
    current = virtual_parent;
    virtual_parent =
        static_cast<BlockElement *>(virtual_parent->virtual_parent());
  }

  size_t base = current->GetFlattenStartIndexInParent();
  return base + local_offset;
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

void BlockElement::RemoveBlockChildAt(size_t index) {
  if (index >= 0 && index < block_children_.size()) {
    block_children_.erase(block_children_.begin() + index);
  }
}

size_t BlockElement::IndexOfBlockChild(const fml::RefPtr<FiberElement> &child) {
  for (size_t index = 0; index < block_children_.size(); ++index) {
    if (block_children_[index].get() == child.get()) {
      return index;
    }
  }
  return static_cast<size_t>(-1);
}

void BlockElement::RemoveBlockChildrenFromParent(BlockElement *block,
                                                 FiberElement *parent) {
  for (const auto &block_child : block->block_children_) {
    if (block_child->is_block()) {
      RemoveBlockChildrenFromParent(
          static_cast<BlockElement *>(block_child.get()), parent);
    } else {
      parent->RemoveNodeInternal(block_child, false, false);
    }
  }
}

void BlockElement::InsertBlockChildrenBefore(BlockElement *block,
                                             FiberElement *parent,
                                             FiberElement *ref) {
  for (const auto &block_child : block->block_children_) {
    block_child->set_virtual_parent(block);
    if (block_child->is_block()) {
      block_child->set_parent(parent);
      InsertBlockChildrenBefore(static_cast<BlockElement *>(block_child.get()),
                                parent, ref);
    } else {
      if (ref) {
        parent->InsertNodeBeforeInternal(block_child, ref, false);
      } else {
        size_t index = block->FindInsertIndex(block_child);
        auto *ref_node =
            index < parent->children().size()
                ? static_cast<FiberElement *>(parent->GetChildAt(index))
                : nullptr;
        parent->InsertNodeBeforeInternal(block_child, ref_node, false);
      }
    }
  }
}

void BlockElement::ReplaceElements(
    const base::Vector<fml::RefPtr<FiberElement>> &inserted,
    const base::Vector<fml::RefPtr<FiberElement>> &removed) {
  // Find the ref node
  FiberElement *last_old_element = nullptr;
  if (!removed.empty()) {
    for (auto iter = removed.rbegin(); iter != removed.rend(); ++iter) {
      FiberElement *element = (*iter).get();
      if (!element->is_block()) {
        last_old_element = element;
        break;
      }
      FiberElement *last_child =
          static_cast<BlockElement *>(element)->LastFlattenedNode();
      if (last_child) {
        last_old_element = last_child;
        break;
      }
    }
  }
  auto *ref =
      last_old_element
          ? static_cast<FiberElement *>(last_old_element->next_sibling())
          : nullptr;
  // Remove elements present in 'removed' but missing from 'inserted'.
  std::unordered_set<fml::RefPtr<FiberElement>> inserted_set(inserted.begin(),
                                                             inserted.end());
  for (const auto &child : removed) {
    if (inserted_set.find(child) == inserted_set.end()) {
      RemoveNode(child);
    }
  }

  if (!parent_) {
    if (removed.empty()) {
      for (const auto &child : inserted) {
        InsertNode(child);
      }
    }
    return;
  }

  FiberElement *parent = static_cast<FiberElement *>(parent_);

  std::unordered_set<fml::RefPtr<FiberElement>> removed_set(removed.begin(),
                                                            removed.end());

  // Traverse 'inserted', detach elements needing movement, then re-insert.
  for (const auto &child : inserted) {
    // Detach element if it was previously removed.
    if (removed_set.count(child)) {
      size_t index = IndexOfBlockChild(child);
      if (index != static_cast<size_t>(-1)) {
        RemoveBlockChildAt(index);
      }

      if (child->is_block()) {
        BlockElement *block_child = static_cast<BlockElement *>(child.get());
        if (parent->scoped_virtual_children_.has_value()) {
          parent->scoped_virtual_children_->erase(
              std::remove(parent->scoped_virtual_children_->begin(),
                          parent->scoped_virtual_children_->end(), child),
              parent->scoped_virtual_children_->end());
        }
        RemoveBlockChildrenFromParent(block_child, parent);
      } else {
        parent->RemoveNodeInternal(child, false, false);
      }

      child->set_virtual_parent(nullptr);
    }

    // Perform re-insertion.
    child->set_virtual_parent(this);

    if (child->is_block()) {
      BlockElement *block_child = static_cast<BlockElement *>(child.get());
      AddBlockChildAt(child, block_children_.size());
      child->set_parent(parent);
      child->set_virtual_parent(this);
      parent->scoped_virtual_children_->push_back(child);
      InsertBlockChildrenBefore(block_child, parent, ref);
    } else {
      if (ref) {
        parent->InsertNodeBeforeInternal(child, ref, false);
      } else {
        size_t index = FindInsertIndex(child);
        auto *ref_node =
            index < parent->children().size()
                ? static_cast<FiberElement *>(parent->GetChildAt(index))
                : nullptr;
        parent->InsertNodeBeforeInternal(child, ref_node, false);
      }
      AddBlockChildAt(child, block_children_.size());
    }
  }
}

FiberElement *BlockElement::FirstFlattenedNode() {
  for (const auto &child : block_children_) {
    auto *fiber_child = static_cast<FiberElement *>(child.get());
    if (!fiber_child->is_block()) {
      return fiber_child;
    }

    auto *first =
        static_cast<BlockElement *>(fiber_child)->FirstFlattenedNode();
    if (first != nullptr) {
      return first;
    }
  }
  return nullptr;
}

FiberElement *BlockElement::FindNearestRightLogicalRenderedSibling(
    FiberElement *parent) {
  const auto &logical_children = parent->logical_children();
  auto it = std::find_if(logical_children.begin(), logical_children.end(),
                         [this](const fml::RefPtr<Element> &logical_child) {
                           return logical_child.get() == this;
                         });
  if (it == logical_children.end()) {
    return nullptr;
  }

  for (++it; it != logical_children.end(); ++it) {
    auto *sibling = static_cast<FiberElement *>((*it).get());
    if (!sibling->is_block()) {
      return sibling;
    }

    auto *first = static_cast<BlockElement *>(sibling)->FirstFlattenedNode();
    if (first != nullptr) {
      return first;
    }
  }

  return nullptr;
}

FiberElement *BlockElement::FindNearestLeftLogicalRenderedSibling(
    FiberElement *parent) {
  const auto &logical_children = parent->logical_children();
  auto it = std::find_if(logical_children.begin(), logical_children.end(),
                         [this](const fml::RefPtr<Element> &logical_child) {
                           return logical_child.get() == this;
                         });
  if (it == logical_children.end()) {
    return nullptr;
  }

  while (it != logical_children.begin()) {
    --it;
    auto *sibling = static_cast<FiberElement *>((*it).get());
    if (!sibling->is_block()) {
      return sibling;
    }

    auto *last = static_cast<BlockElement *>(sibling)->LastFlattenedNode();
    if (last != nullptr) {
      return last;
    }
  }

  return nullptr;
}

size_t BlockElement::GetFlattenStartIndexInParent() {
  auto *parent = static_cast<FiberElement *>(parent_);
  auto *first = FirstFlattenedNode();

  if (first != nullptr) {
    int32_t first_index = parent->IndexOf(first);
    if (first_index >= 0) {
      return static_cast<size_t>(first_index);
    }
  }

  if (auto *right = FindNearestRightLogicalRenderedSibling(parent)) {
    int32_t right_index = parent->IndexOf(right);
    if (right_index >= 0) {
      return static_cast<size_t>(right_index);
    }
  }

  if (auto *left = FindNearestLeftLogicalRenderedSibling(parent)) {
    int32_t left_index = parent->IndexOf(left);
    if (left_index >= 0) {
      return static_cast<size_t>(left_index + 1);
    }
  }

  return parent->children().size();
}

FiberElement *BlockElement::LastFlattenedNode() {
  for (auto it = block_children_.rbegin(); it != block_children_.rend(); ++it) {
    auto *fiber_child = static_cast<FiberElement *>((*it).get());
    if (!fiber_child->is_block()) {
      return fiber_child;
    }

    auto *last = static_cast<BlockElement *>(fiber_child)->LastFlattenedNode();
    if (last != nullptr) {
      return last;
    }
  }
  return nullptr;
}
}  // namespace tasm
}  // namespace lynx
