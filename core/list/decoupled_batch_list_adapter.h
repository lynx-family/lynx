// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_BATCH_LIST_ADAPTER_H_
#define CORE_LIST_DECOUPLED_BATCH_LIST_ADAPTER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "core/list/decoupled_list_adapter.h"
#include "core/list/list_item_element_delegate.h"

namespace lynx {
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

using ListItemDelegateMap =
    std::unordered_map<std::string, ItemElementDelegate*>;
using ItemStatusMap = std::unordered_map<std::string, ItemStatus>;
using BindingKeyMap = std::unordered_map<int64_t, std::string>;

class BatchListAdapter : public ListAdapter {
 public:
  BatchListAdapter(ListContainerImpl* list_container_impl)
      : ListAdapter(list_container_impl),
        list_item_delegate_map_(std::make_unique<ListItemDelegateMap>()),
        item_status_map_(std::make_unique<ItemStatusMap>()),
        binding_key_map_(std::make_unique<BindingKeyMap>()) {}

  ~BatchListAdapter() override = default;

  BatchListAdapter(const BatchListAdapter& rhs) = delete;

  BatchListAdapter& operator=(const BatchListAdapter& rhs) = delete;

  BatchListAdapter(ListAdapter&& rhs) noexcept : ListAdapter(std::move(rhs)) {
    list_item_delegate_map_ = std::make_unique<ListItemDelegateMap>();
    item_status_map_ = std::make_unique<ItemStatusMap>();
    binding_key_map_ = std::make_unique<BindingKeyMap>();
  }

  BatchListAdapter& operator=(ListAdapter&& rhs) noexcept {
    if (this != &rhs) {
      ListAdapter::operator=(std::move(rhs));
      list_item_delegate_map_ = std::make_unique<ListItemDelegateMap>();
      item_status_map_ = std::make_unique<ItemStatusMap>();
      binding_key_map_ = std::make_unique<BindingKeyMap>();
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
  void OnItemHolderUpdateTo(ItemHolder* item_holder, bool fiber_flush) override;

  // Handle diff moved from
  void OnItemHolderMovedFrom(ItemHolder* item_holder) override {}

  // Handle diff moved from
  void OnItemHolderMovedTo(ItemHolder* item_holder) override {}

  // Handle diff remove and insert again.
  void OnItemHolderReInsert(ItemHolder* item_holder) override;

  void OnEnqueueElement(ItemHolder* item_holder) override;

#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event,
                            ItemHolder* item_holder) const override;
#endif

 public:
  // Handle full data updated.
  void OnDataSetChanged() override;

  // Bind the item holder with index.
  bool BindItemHolder(ItemHolder* item_holder, int index,
                      bool preload_section = false) override;

  // Bind item holders in the set.
  void BindItemHolders(const ItemHolderSet& item_holder_set) override;

  // Finish bind item holder with element.
  void OnFinishBindItemHolder(
      ItemElementDelegate* list_item_delegate,
      const std::shared_ptr<tasm::PipelineOptions>& options) override;

  // Finish bind item holders with elements. Note: no need to implement.
  void OnFinishBindItemHolders(
      const std::vector<ItemElementDelegate*>& list_item_delegate_array,
      const std::shared_ptr<tasm::PipelineOptions>& options) override;

  // Recycle ItemHolder.
  void RecycleItemHolder(ItemHolder* item_holder) override;

  // Return whether the ItemHolder has already been bound, if return true, it
  // means the ItemHolder is a no dirty node, but with no valid list item
  // element.
  bool IsRecycled(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(), ItemStatus::kRecycled);
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
                           ItemStatus::kFinishedBinding);
  }

  // Return whether the ItemHolder is dirty
  bool IsDirty(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(), ItemStatus::kNeverBind) ||
           CheckItemStatus(item_holder->item_key(), ItemStatus::kUpdated);
  }

  // Return whether the ItemHolder is update_to
  bool IsUpdated(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(), ItemStatus::kUpdated);
  }

  // Return whether the ItemHolder is removed
  bool IsRemoved(const ItemHolder* item_holder) override {
    return CheckItemStatus(item_holder->item_key(), ItemStatus::kRemoved);
  }

  ItemElementDelegate* GetItemElementDelegate(
      const ItemHolder* item_holder) override {
    return GetItemElementDelegate(item_holder->item_key());
  }

 private:
  int64_t BindItemHolderInternal(ItemHolder* item_holder, int index,
                                 bool invoke_bind = true);

  int OnFinishBindInternal(ItemElementDelegate* list_item_delegate,
                           int64_t operation_id);

  int OnFinishValidBind(const std::string& item_key,
                        ItemElementDelegate* list_item_delegate);

  void MarkItemStatus(const std::string& item_key, uint32_t item_status);

  bool CheckItemStatus(const std::string& item_key, uint32_t item_status) const;

  ItemElementDelegate* GetItemElementDelegate(
      const std::string& item_key) const {
    auto it = list_item_delegate_map_->find(item_key);
    if (it != list_item_delegate_map_->end()) {
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

  bool IsBinding(const ItemStatus& item_status) const {
    return item_status.operation_id_ != 0;
  }

  bool IsFinishedBinding(const ItemStatus& item_status) const {
    return item_status == ItemStatus::kFinishedBinding;
  }

  bool IsDirty(const ItemStatus& item_status) const {
    return item_status == ItemStatus::kNeverBind ||
           item_status == ItemStatus::kUpdated;
  }

  bool IsRecycled(const ItemStatus& item_status) const {
    return item_status == ItemStatus::kRecycled;
  }

  bool IsRemoved(const ItemStatus& item_status) const {
    return item_status == ItemStatus::kRemoved;
  }

  bool IsNeverBind(const ItemStatus& item_status) const {
    return item_status == ItemStatus::kNeverBind;
  }

 private:
  // <item-key, ItemDelegate*> map
  std::unique_ptr<ListItemDelegateMap> list_item_delegate_map_;
  // <item-key, ItemStatus> map
  std::unique_ptr<ItemStatusMap> item_status_map_;
  // <operation-id, item-key> map
  std::unique_ptr<BindingKeyMap> binding_key_map_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_BATCH_LIST_ADAPTER_H_
