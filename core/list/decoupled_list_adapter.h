// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_ADAPTER_H_
#define CORE_LIST_DECOUPLED_LIST_ADAPTER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/list/decoupled_adapter_helper.h"
#include "core/list/decoupled_item_holder.h"
#include "core/list/decoupled_list_children_helper.h"
#include "core/public/pipeline_option.h"

namespace lynx {
namespace list {

class ListContainerImpl;

class ListAdapter : public AdapterHelper::Delegate {
 public:
  ListAdapter(ListContainerImpl* list_container_impl);

  virtual ~ListAdapter() = default;

  ListAdapter(const ListAdapter& rhs) = delete;

  ListAdapter& operator=(const ListAdapter& rhs) = delete;

  ListAdapter(ListAdapter&& rhs) noexcept
      : list_container_(rhs.list_container_),
        item_holder_map_(std::move(rhs.item_holder_map_)),
        adapter_helper_(std::move(rhs.adapter_helper_)) {
    adapter_helper_->SetDelegate(this);
    rhs.Release();
  }

  ListAdapter& operator=(ListAdapter&& rhs) noexcept {
    if (this != &rhs) {
      list_container_ = rhs.list_container_;
      item_holder_map_ = std::move(rhs.item_holder_map_);
      adapter_helper_ = std::move(rhs.adapter_helper_);
      adapter_helper_->SetDelegate(this);
      rhs.Release();
    }
    return *this;
  }

  void OnErrorOccurred(lynx::base::LynxError error) override;

 protected:
  // Handle diff insert.
  virtual void OnItemHolderInserted(ItemHolder* item_holder) = 0;

  // Handle diff removed.
  virtual void OnItemHolderRemoved(ItemHolder* item_holder) = 0;

  // Handle diff update from
  virtual void OnItemHolderUpdateFrom(ItemHolder* item_holder) = 0;

  // Handle diff update to
  virtual void OnItemHolderUpdateTo(ItemHolder* item_holder,
                                    bool fiber_flush) = 0;

  // Handle diff moved from
  virtual void OnItemHolderMovedFrom(ItemHolder* item_holder) = 0;

  // Handle diff moved from
  virtual void OnItemHolderMovedTo(ItemHolder* item_holder) = 0;

  // Handle diff remove and insert again.
  virtual void OnItemHolderReInsert(ItemHolder* item_holder) = 0;

  virtual void OnEnqueueElement(ItemHolder* item_holder) = 0;

#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event) const;

  virtual void UpdateTraceDebugInfo(TraceEvent* event,
                                    ItemHolder* item_holder) const;
#endif

 public:
  // Handle full data updated.
  virtual void OnDataSetChanged() = 0;

  // Bind the item holder with index.
  virtual bool BindItemHolder(ItemHolder* item_holder, int index,
                              bool preload_section = false) = 0;

  // Bind item holders in the set.
  virtual void BindItemHolders(const ItemHolderSet& item_holder_set) = 0;

  // Finish bind item holder with element.
  virtual void OnFinishBindItemHolder(
      ItemElementDelegate* list_item_delegate,
      const std::shared_ptr<tasm::PipelineOptions>& options) = 0;

  // Finish bind item holders with elements
  virtual void OnFinishBindItemHolders(
      const std::vector<ItemElementDelegate*>& list_items,
      const std::shared_ptr<tasm::PipelineOptions>& options) = 0;

  // Recycle ItemHolder.
  virtual void RecycleItemHolder(ItemHolder* item_holder) = 0;

  // Return whether the ItemHolder has already been bound, if return true, it
  // means the ItemHolder is a no dirty node, but with no valid list item
  // element.
  virtual bool IsRecycled(const ItemHolder* item_holder) = 0;

  // Return whether the ItemHolder is in binding.
  virtual bool IsBinding(const ItemHolder* item_holder) = 0;

  // Return whether the ItemHolder is in finish binding, if return true, it
  // means the ItemHolder is a no dirty node with valid list item element.
  virtual bool IsFinishedBinding(const ItemHolder* item_holder) = 0;

  // Return whether the ItemHolder is dirty
  virtual bool IsDirty(const ItemHolder* item_holder) = 0;

  // Return whether the ItemHolder is update_to
  virtual bool IsUpdated(const ItemHolder* item_holder) = 0;

  // Return whether the ItemHolder is removed
  virtual bool IsRemoved(const ItemHolder* item_holder) = 0;

  virtual ItemElementDelegate* GetItemElementDelegate(
      const ItemHolder* item_holder) = 0;

  virtual void RecycleItemHolder(const ItemHolder* item_holder) {}

  // Return diff result and whether to use animation in this diff process,
  // animation will not be enabled in first screen.
  std::pair<ListAdapterDiffResult, bool> UpdateRadonDataSource(
      const pub::Value& radon_data_source);

  std::pair<ListAdapterDiffResult, bool> UpdateFiberDataSource(
      const pub::Value& fiber_data_source);

  // Return parsed diff result.
  std::unique_ptr<pub::Value> GenerateDiffResult() const;

  void ClearDiffResult();

  void UpdateItemHolderToLatest(ListChildrenHelper* list_children_helper);

  // Recycle all itemHolders when basic list props changed such as
  // column-count/list-type.
  void RecycleAllItemHolders();

  // Recycle all removed ItemHolders.
  void RecycleRemovedItemHolders();

  // If the list item is self-layout-updated, we invoke the method to update
  // layout info to the ItemHolder.
  void UpdateLayoutInfoToItemHolder(ItemElementDelegate* list_item_delegate,
                                    ItemHolder* item_holder);

  void EnqueueElementsIfNeeded();

  ItemHolder* GetItemHolderForIndex(int index);

  bool IsFullSpanAtIndex(int index) const;

  void Release() {
    list_container_ = nullptr;
    adapter_helper_ = nullptr;
    item_holder_map_ = nullptr;
  }

  int GetDataCount() const {
    return adapter_helper_ ? adapter_helper_->GetDateCount() : 0;
  }

  bool HasFullSpanItems() { return !adapter_helper_->full_spans().empty(); }

  const std::vector<int32_t>& GetStickyTops() const {
    return adapter_helper_->sticky_tops();
  }

  const std::vector<int32_t>& GetStickyBottoms() const {
    return adapter_helper_->sticky_bottoms();
  }

  bool IsRecyclableAtIndex(int index) const;

  AdapterHelper* list_adapter_helper() const { return adapter_helper_.get(); }

  const std::unique_ptr<ItemHolderMap>& item_holder_map() const {
    return item_holder_map_;
  }

 protected:
  int64_t GenerateOperationId() const;

  // It will invoked list's EnqueueComponent() to recycle component
  // bound with ItemHolder and remove platform view from parent.
  void EnqueueElement(ItemHolder* item_holder);

 private:
  float GetEstimatedSizeForIndex(int index);

  void MarkChildHolderDirty();

  void CheckSticky(ItemHolder* item_holder, int32_t index);

  bool HasExpectedDiffAnimation() const;

  void UpdateAnchorRefItem(ListChildrenHelper* list_children_helper);

  void GenerateAndFlushListContainerInfo();

  void GenerateDiffArray(const std::string& diff_key,
                         const std::vector<int32_t>& diff_array,
                         const std::unique_ptr<pub::Value>& diff_result) const;

 protected:
  ListContainerImpl* list_container_{nullptr};
  std::unique_ptr<ItemHolderMap> item_holder_map_;
  ItemHolderSet fiber_flush_item_holder_set_;

 private:
  std::unique_ptr<AdapterHelper> adapter_helper_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_ADAPTER_H_
