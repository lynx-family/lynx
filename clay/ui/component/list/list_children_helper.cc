// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_children_helper.h"

#include <functional>
#include <string>

#include "clay/fml/logging.h"
#include "clay/ui/component/list/list_item_view_holder.h"

namespace clay {

ListChildrenHelper::ListChildrenHelper() = default;
ListChildrenHelper::~ListChildrenHelper() = default;

int ListChildrenHelper::GetChildCount() const {
  return static_cast<int>(children_.size());
}

ListItemViewHolder* ListChildrenHelper::GetChildAt(int index) {
  if (index < 0 || index >= GetChildCount()) {
    return nullptr;
  }
  return *GetChildIterator(index);
}

void ListChildrenHelper::AddChild(ListItemViewHolder* item, int index) {
  FML_DCHECK(index >= 0 && index <= GetChildCount());
  FML_DCHECK(floating_top_item_ != item && floating_bottom_item_ != item);
  ViewHolderList::iterator itr = GetChildIterator(index);
  children_.insert(itr, item);
}

ListItemViewHolder* ListChildrenHelper::RemoveChild(int index) {
  FML_DCHECK(index >= 0 && index < GetChildCount());
  ViewHolderList::iterator itr = GetChildIterator(index);
  ListItemViewHolder* child = *itr;
  children_.erase(itr);
  return child;
}

ListItemViewHolder* ListChildrenHelper::FindChildByPosition(int position) {
  if (GetChildCount() == 0) {
    return nullptr;
  }

  // We assume children's position is continuous.
  const ListItemViewHolder* first = GetChildAt(0);
  const int first_pos = first->GetPosition();
  if (position < first_pos || position >= first_pos + GetChildCount()) {
    return nullptr;
  }
  return GetChildAt(position - first_pos);
}

void ListChildrenHelper::ForEach(
    const std::function<bool(ListItemViewHolder*)>& func) {
  FML_DCHECK(func);
  for (ListItemViewHolder* view_holder : children_) {
    if (func(view_holder)) {
      break;
    }
  }
}

void ListChildrenHelper::ReversedForEach(
    const std::function<bool(ListItemViewHolder*)>& func) {
  FML_DCHECK(func);
  auto iter = children_.rbegin();
  for (; iter != children_.rend(); ++iter) {
    if (func(*iter)) {
      break;
    }
  }
}

void ListChildrenHelper::ForEachInPositions(
    const std::vector<int>& positions,
    const std::function<bool(ListItemViewHolder*)>& func) {
  FML_DCHECK(func);
  ListChildrenHelper::ViewHolderList::iterator iter;
  bool last_in_children = false;
  for (size_t i = 0; i < positions.size(); ++i) {
    int position = positions[i];
    if (floating_top_item_ && position == floating_top_item_->GetPosition()) {
      if (func(floating_top_item_)) {
        return;
      }
      last_in_children = false;
    } else if (floating_bottom_item_ &&
               position == floating_bottom_item_->GetPosition()) {
      if (func(floating_bottom_item_)) {
        return;
      }
      last_in_children = false;
    } else {
      if (last_in_children && positions[i - 1] + 1 == position) {
        // Continuous, forward the iterator
        ++iter;
      } else {
        // Not continuous, find iterator by index
        int index_in_item_list = GetChildIndexByPosition(positions[i]);
        if (index_in_item_list == ListItemViewHolder::kNoPosition) {
          FML_DLOG(ERROR) << "Unable to find child at position "
                          << positions[i];
          // If we can't find the front item, then can't find remaining either.
          return;
        }
        iter = GetChildIterator(index_in_item_list);
        last_in_children = true;
      }
      if (func(*iter)) {
        return;
      }
    }
  }
}

void ListChildrenHelper::ForEachWithSticky(
    const std::function<bool(ListItemViewHolder*)>& func) {
  FML_DCHECK(func);
  if (floating_top_item_) {
    if (func(floating_top_item_)) {
      return;
    }
  }
  for (ListItemViewHolder* view_holder : children_) {
    if (func(view_holder)) {
      return;
    }
  }
  if (floating_bottom_item_) {
    func(floating_bottom_item_);
  }
}

ListChildrenHelper::ViewHolderList::iterator
ListChildrenHelper::GetChildIterator(int index) {
  const int count = GetChildCount();
  FML_DCHECK(index >= 0 && index <= count);
  if (index > count / 2) {
    auto itr = children_.rbegin();
    for (int i = count; i > index; --i) {
      ++itr;
    }
    return itr.base();
  } else {
    auto itr = children_.begin();
    for (int i = 0; i < index; ++i) {
      ++itr;
    }
    return itr;
  }
}

int ListChildrenHelper::GetChildIndexByPosition(int position) {
  if (GetChildCount() == 0) {
    return ListItemViewHolder::kNoPosition;
  }
  const ListItemViewHolder* first = GetChildAt(0);
  const int first_pos = first->GetPosition();

  if (position < first_pos || position >= first_pos + GetChildCount()) {
    return ListItemViewHolder::kNoPosition;
  }
  return position - first_pos;
}

std::string ListChildrenHelper::ToString() const {
#if (DEBUG_LIST)
  std::stringstream ss;
  if (floating_top_item_) {
    ss << std::endl << "[floating_top]" << floating_top_item_->ToString();
  }
  unsigned int idx = 0;
  for (auto const& child : children_) {
    ss << std::endl << "[" << idx << "] " << child->ToString();
    idx++;
  }
  if (floating_bottom_item_) {
    ss << std::endl
       << "[floating_bottom] " << floating_bottom_item_->ToString();
  }
  return ss.str();
#else
  return "";
#endif
}

}  // namespace clay
