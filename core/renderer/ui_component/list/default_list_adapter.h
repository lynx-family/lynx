// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_DEFAULT_LIST_ADAPTER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_DEFAULT_LIST_ADAPTER_H_

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/ui_component/list/list_adapter.h"

namespace lynx {
namespace tasm {

namespace list {
using BindingItemHolderMap = std::unordered_map<int64_t, ItemHolder*>;
}

class DefaultListAdapter : public ListAdapter {
 public:
  DefaultListAdapter(ListContainerImpl* list_container_impl, Element* element)
      : ListAdapter(list_container_impl, element),
        binding_item_holder_map_(
            std::make_unique<list::BindingItemHolderMap>()) {}

  ~DefaultListAdapter() override = default;

  DefaultListAdapter(const DefaultListAdapter& rhs) = delete;

  DefaultListAdapter& operator=(const DefaultListAdapter& rhs) = delete;

  DefaultListAdapter(ListAdapter&& rhs) noexcept
      : ListAdapter(std::move(rhs)) {}

  DefaultListAdapter& operator=(ListAdapter&& rhs) noexcept {
    if (this != &rhs) {
      ListAdapter::operator=(std::move(rhs));
    }
    return *this;
  }

 protected:
  // Handle diff insert.
  void OnItemHolderInserted(ItemHolder* item_holder) override {}

  // Handle diff removed.
  void OnItemHolderRemoved(ItemHolder* item_holder) override;

  // Handle diff update from
  void OnItemHolderUpdateFrom(ItemHolder* item_holder) override;

  // Handle diff update to
  void OnItemHolderUpdateTo(ItemHolder* item_holder) override;

  // Handle diff moved from
  void OnItemHolderMovedFrom(ItemHolder* item_holder) override;

  // Handle diff moved to
  void OnItemHolderMovedTo(ItemHolder* item_holder) override;

  // Handle diff remove and insert again.
  void OnItemHolderReInsert(ItemHolder* item_holder) override;

 public:
  // Handle full data updated.
  void OnDataSetChanged() override;

  // Bind the item holder with index.
  bool BindItemHolder(ItemHolder* item_holder, int index,
                      bool preload_section = false) override;

  // Bind item holders in the set. Note: no need to implement.
  void BindItemHolders(const ItemHolderSet& item_holder_set) override {}

  // Finish bind item holder with element.
  void OnFinishBindItemHolder(Element* component,
                              const PipelineOptions& option) override;

  // Finish bind item holders with elements. Note: no need to implement.
  void OnFinishBindItemHolders(const std::vector<Element*>& components,
                               const PipelineOptions& option) override {}

  // Recycle ItemHolder.
  void RecycleItemHolder(ItemHolder* item_holder) override;

  // Return whether the ItemHolder has already been bound, if return true, it
  // means the ItemHolder is a no dirty node, but with no valid list item
  // element.
  bool IsRecycled(const ItemHolder* item_holder) override {
    return !item_holder->dirty_ && item_holder->operation_id_ == 0 &&
           !item_holder->element_;
  }

  // Return whether the ItemHolder is in binding.
  bool IsBinding(const ItemHolder* item_holder) override {
    return item_holder->operation_id_ != 0;
  }

  // Return whether the ItemHolder is in finish binding, if return true, it
  // means the ItemHolder is a no dirty node with valid list item element.
  bool IsFinishedBinding(const ItemHolder* item_holder) override {
    return !item_holder->dirty_ && item_holder->operation_id_ == 0 &&
           item_holder->element_;
  }

  // Return whether the ItemHolder is dirty
  bool IsDirty(const ItemHolder* item_holder) override {
    return item_holder->dirty_;
  }

  // Return whether the ItemHolder is update_to
  bool IsUpdated(const ItemHolder* item_holder) override {
    return item_holder->dirty_ && item_holder->is_updated();
  }

  // Return whether the ItemHolder is removed
  bool IsRemoved(const ItemHolder* item_holder) override {
    return item_holder->removed_;
  }

  Element* GetListItemElement(const ItemHolder* item_holder) override {
    return item_holder->element_;
  }

 private:
  std::unique_ptr<list::BindingItemHolderMap> binding_item_holder_map_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_DEFAULT_LIST_ADAPTER_H_
