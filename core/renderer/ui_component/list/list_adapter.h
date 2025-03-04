// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_ADAPTER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_ADAPTER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/ui_component/list/adapter_helper.h"
#include "core/renderer/ui_component/list/item_holder.h"
#include "core/renderer/ui_component/list/list_children_helper.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"

namespace lynx {
namespace tasm {

class ListContainerImpl;

class ListAdapter : public AdapterHelper::Delegate {
 public:
  ListAdapter(ListContainerImpl* list_container_impl, Element* element);

  virtual ~ListAdapter() = default;

  ListAdapter(const ListAdapter& rhs) = delete;

  ListAdapter& operator=(const ListAdapter& rhs) = delete;

  ListAdapter(ListAdapter&& rhs) noexcept
      : list_element_(rhs.list_element_),
        list_container_(rhs.list_container_),
        item_holder_map_(std::move(rhs.item_holder_map_)),
        adapter_helper_(std::move(rhs.adapter_helper_)) {
    rhs.Release();
  }

  ListAdapter& operator=(ListAdapter&& rhs) noexcept {
    if (this != &rhs) {
      list_element_ = rhs.list_element_;
      list_container_ = rhs.list_container_;
      item_holder_map_ = std::move(rhs.item_holder_map_);
      adapter_helper_ = std::move(rhs.adapter_helper_);
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
  virtual void OnItemHolderUpdateTo(ItemHolder* item_holder) = 0;

  // Handle diff moved from
  virtual void OnItemHolderMovedFrom(ItemHolder* item_holder) = 0;

  // Handle diff moved from
  virtual void OnItemHolderMovedTo(ItemHolder* item_holder) = 0;

  // Handle diff remove and insert again.
  virtual void OnItemHolderReInsert(ItemHolder* item_holder) = 0;

 public:
  // Handle full data updated.
  virtual void OnDataSetChanged() = 0;

  // Bind the item holder with index.
  virtual bool BindItemHolder(ItemHolder* item_holder, int index,
                              bool preload_section = false) = 0;

  // Bind item holders in the set.
  virtual void BindItemHolders(const ItemHolderSet& item_holder_set) = 0;

  // Finish bind item holder with element.
  virtual void OnFinishBindItemHolder(Element* component,
                                      const PipelineOptions& option) = 0;

  // Finish bind item holders with elements
  virtual void OnFinishBindItemHolders(const std::vector<Element*>& list_items,
                                       const PipelineOptions& options) = 0;

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

  virtual Element* GetListItemElement(const ItemHolder* item_holder) = 0;

#if ENABLE_TRACE_PERFETTO
  void UpdateTraceDebugInfo(TraceEvent* event);
#endif

  bool UpdateDataSource(const lepus::Value& data_source);

  bool UpdateFiberDataSource(const lepus::Value& data_source);

  void UpdateListContainerDataSource(
      fml::RefPtr<lepus::Dictionary>& data_source);

  void UpdateItemHolderToLatest(ListChildrenHelper* list_children_helper);

  // Recycle all itemHolders when basic list props changed such as
  // column-count/list-type.
  void RecycleAllItemHolders();

  // Recycle all removed ItemHolders.
  void RecycleRemovedItemHolders();

  // If the list item is self-layout-updated, we invoke the method to update
  // layout info to the ItemHolder.
  void UpdateLayoutInfoToItemHolder(Element* list_item,
                                    ItemHolder* item_holder);

  ItemHolder* GetItemHolderForIndex(int index);

  bool IsFullSpanAtIndex(int index);

  void Release() {
    list_element_ = nullptr;
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

  AdapterHelper* list_adapter_helper() const { return adapter_helper_.get(); }

  const std::unique_ptr<list::ItemHolderMap>& item_holder_map() const {
    return item_holder_map_;
  }

 protected:
  int64_t GenerateOperationId() const;

 private:
  float GetEstimatedSizeForIndex(int index);

  void MarkChildHolderDirty();

  void CheckSticky(ItemHolder* item_holder, int32_t index);

 protected:
  Element* list_element_{nullptr};
  ListContainerImpl* list_container_{nullptr};
  std::unique_ptr<list::ItemHolderMap> item_holder_map_;

 private:
  std::unique_ptr<AdapterHelper> adapter_helper_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_ADAPTER_H_
