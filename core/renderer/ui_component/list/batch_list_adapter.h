// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_BATCH_LIST_ADAPTER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_BATCH_LIST_ADAPTER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/ui_component/list/list_adapter.h"

namespace lynx {
namespace tasm {

class Element;

namespace list {
class ItemStatus {
 public:
  static const uint32_t kNeverBind;
  static const uint32_t kUpdated;
  static const uint32_t kRemoved;
  static const uint32_t kInBinding;
  static const uint32_t kFinishedBinding;
  static const uint32_t kRecycled;

  ItemStatus(uint32_t status = kNeverBind)
      : status_(status), operation_id_(0) {}

  std::string ToString() const {
    return base::FormatString("ItemStatus[status_=%d, operation_id_=%ld]",
                              status_, operation_id_);
  }

  bool operator==(const ItemStatus& rhs) const {
    return status_ == rhs.status_;
  }

  bool operator!=(const ItemStatus& rhs) const { return !operator==(rhs); }

  uint32_t status_{kNeverBind};
  int64_t operation_id_{0};
};

using ListItemElementMap = std::unordered_map<std::string, Element*>;
using ItemStatusMap = std::unordered_map<std::string, list::ItemStatus>;
using BindingKeyMap = std::unordered_map<int64_t, std::string>;
}  // namespace list

class BatchListAdapter : public ListAdapter {
 public:
  BatchListAdapter(ListContainerImpl* list_container_impl, Element* element)
      : ListAdapter(list_container_impl, element),
        list_item_element_map_(std::make_unique<list::ListItemElementMap>()),
        item_status_map_(std::make_unique<list::ItemStatusMap>()),
        binding_key_map_(std::make_unique<list::BindingKeyMap>()) {}

  ~BatchListAdapter() override = default;

  BatchListAdapter(const BatchListAdapter& rhs) = delete;

  BatchListAdapter& operator=(const BatchListAdapter& rhs) = delete;

  BatchListAdapter(ListAdapter&& rhs) noexcept : ListAdapter(std::move(rhs)) {
    list_item_element_map_ = std::make_unique<list::ListItemElementMap>();
    item_status_map_ = std::make_unique<list::ItemStatusMap>();
    binding_key_map_ = std::make_unique<list::BindingKeyMap>();
  }

  BatchListAdapter& operator=(ListAdapter&& rhs) noexcept {
    if (this != &rhs) {
      ListAdapter::operator=(std::move(rhs));
      list_item_element_map_ = std::make_unique<list::ListItemElementMap>();
      item_status_map_ = std::make_unique<list::ItemStatusMap>();
      binding_key_map_ = std::make_unique<list::BindingKeyMap>();
    }
    return *this;
  }

 protected:
  // Handle diff insert.
  void OnItemHolderInserted(ItemHolder* item_holder) override;

  // Handle diff removed.
  void OnItemHolderRemoved(ItemHolder* item_holder) override;

  // Handle diff update from
  void OnItemHolderUpdateFrom(ItemHolder* item_holder) override {}

  // Handle diff update to
  void OnItemHolderUpdateTo(ItemHolder* item_holder) override;

  // Handle diff moved from
  void OnItemHolderMovedFrom(ItemHolder* item_holder) override {}

  // Handle diff moved from
  void OnItemHolderMovedTo(ItemHolder* item_holder) override {}

  // Handle diff remove and insert again.
  void OnItemHolderReInsert(ItemHolder* item_holder) override;

 public:
  // Handle full data updated.
  void OnDataSetChanged() override;

  // Bind the item holder with index.
  bool BindItemHolder(ItemHolder* item_holder, int index,
                      bool preload_section = false) override;

  // Bind item holders in the set.
  void BindItemHolders(const ItemHolderSet& item_holder_set) override;

  // Finish bind item holder with element.
  void OnFinishBindItemHolder(Element* list_item,
                              const PipelineOptions& option) override;

  // Finish bind item holders with elements
  void OnFinishBindItemHolders(const std::vector<Element*>& list_items,
                               const PipelineOptions& options) override;

  // Recycle ItemHolder.
  void RecycleItemHolder(ItemHolder* item_holder) override;

  // Return whether the ItemHolder has already been bound, if return true, it
  // means the ItemHolder is a no dirty node, but with no valid list item
  // element.
  bool IsRecycled(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(),
                           list::ItemStatus::kRecycled);
  }

  // Return whether the ItemHolder is in binding.
  bool IsBinding(const ItemHolder* item_holder) override {
    auto it = item_status_map_->find(item_holder->item_key());
    if (it != item_status_map_->end()) {
      return (it->second).operation_id_ != 0;
    }
    return false;
  }

  // Return whether the ItemHolder is in finish binding, if return true, it
  // means the ItemHolder is a no dirty node with valid list item element.
  bool IsFinishedBinding(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(),
                           list::ItemStatus::kFinishedBinding);
  }

  // Return whether the ItemHolder is dirty
  bool IsDirty(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(),
                           list::ItemStatus::kNeverBind) ||
           CheckItemStatus(item_holder->item_key(), list::ItemStatus::kUpdated);
  }

  // Return whether the ItemHolder is update_to
  bool IsUpdated(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(), list::ItemStatus::kUpdated);
  }

  // Return whether the ItemHolder is removed
  bool IsRemoved(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(), list::ItemStatus::kRemoved);
  }

  Element* GetListItemElement(const ItemHolder* item_holder) override {
    return GetListItemElement(item_holder->item_key());
  }

 private:
  int64_t BindItemHolderInternal(ItemHolder* item_holder, int index,
                                 ListNode* list_node, bool invoke_bind = true);

  int OnFinishBindInternal(Element* list_item, int64_t operation_id,
                           ListNode* list_node);

  int OnFinishValidBind(const std::string& item_key, Element* list_item);

  void MarkItemStatus(const std::string& item_key, uint32_t item_status);

  bool CheckItemStatus(const std::string& item_key, uint32_t item_status) const;

  Element* GetListItemElement(const std::string& item_key) const {
    auto it = list_item_element_map_->find(item_key);
    if (it != list_item_element_map_->end()) {
      return it->second;
    }
    return nullptr;
  }

  bool IsBinding(const std::string& item_key) const {
    auto it = item_status_map_->find(item_key);
    if (it != item_status_map_->end()) {
      return (it->second).operation_id_ != 0;
    }
    return false;
  }

  bool IsBinding(const list::ItemStatus& item_status) const {
    return item_status.operation_id_ != 0;
  }

  bool IsFinishedBinding(const list::ItemStatus& item_status) const {
    return item_status == list::ItemStatus::kFinishedBinding;
  }

  bool IsDirty(const list::ItemStatus& item_status) const {
    return item_status == list::ItemStatus::kNeverBind ||
           item_status == list::ItemStatus::kUpdated;
  }

  bool IsRecycled(const list::ItemStatus& item_status) const {
    return item_status == list::ItemStatus::kRecycled;
  }

  bool IsRemoved(const list::ItemStatus& item_status) const {
    return item_status == list::ItemStatus::kRemoved;
  }

  bool IsNeverBind(const list::ItemStatus& item_status) const {
    return item_status == list::ItemStatus::kNeverBind;
  }

 private:
  // <item-key, Element*> map
  std::unique_ptr<list::ListItemElementMap> list_item_element_map_;
  // <item-key, ItemStatus> map
  std::unique_ptr<list::ItemStatusMap> item_status_map_;
  // <operation-id, item-key> map
  std::unique_ptr<list::BindingKeyMap> binding_key_map_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_BATCH_LIST_ADAPTER_H_
