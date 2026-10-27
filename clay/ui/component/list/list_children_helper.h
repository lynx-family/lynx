// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_CHILDREN_HELPER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_CHILDREN_HELPER_H_

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "clay/fml/logging.h"

namespace clay {

class ListItemViewHolder;

// In Android's recycler view, this is used as the container of the view holders
// and handle the asynchronous states between the adapter and the recycler view
// caused by animations.
// We don't handle animation yet and this class is just used as the container
// for view holders.
class ListChildrenHelper {
 public:
  ListChildrenHelper();
  ~ListChildrenHelper();

  int GetChildCount() const;
  ListItemViewHolder* GetChildAt(int index);
  void AddChild(ListItemViewHolder* item, int index);
  ListItemViewHolder* RemoveChild(int index);
  ListItemViewHolder* FindChildByPosition(int position);

  ListItemViewHolder* floating_top_item() const { return floating_top_item_; }
  ListItemViewHolder* floating_bottom_item() const {
    return floating_bottom_item_;
  }

  void set_floating_top_item(ListItemViewHolder* item) {
    FML_DCHECK(std::find(children_.begin(), children_.end(), item) ==
               children_.end());
    floating_top_item_ = item;
  }
  void set_floating_bottom_item(ListItemViewHolder* item) {
    FML_DCHECK(std::find(children_.begin(), children_.end(), item) ==
               children_.end());
    floating_bottom_item_ = item;
  }

  // Iterate the children list. Stop the iteration when func returns true.
  void ForEach(const std::function<bool(ListItemViewHolder*)>& func);
  void ReversedForEach(const std::function<bool(ListItemViewHolder*)>& func);
  // Iterate a range of position(they may not continuous but must be sorted).
  void ForEachInPositions(const std::vector<int>& positions,
                          const std::function<bool(ListItemViewHolder*)>& func);

  // Iterate all children (and containing sticky items). Stop the iteration when
  // func returns true.
  void ForEachWithSticky(const std::function<bool(ListItemViewHolder*)>& func);

  std::string ToString() const;

 private:
  using ViewHolderList = std::list<ListItemViewHolder*>;
  ViewHolderList::iterator GetChildIterator(int index);

  int GetChildIndexByPosition(int position);

  ViewHolderList children_;

  // If the current header item is not in `children_`, it will be stored here.
  ListItemViewHolder* floating_top_item_ = nullptr;
  // If the current footer item is not in `children_`, it will be stored here.
  ListItemViewHolder* floating_bottom_item_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_CHILDREN_HELPER_H_
