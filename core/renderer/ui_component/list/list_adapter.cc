// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_adapter.h"

#include <unordered_set>

#include "base/include/log/logging.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {

ListAdapter::ListAdapter(ListContainerImpl* list_container_impl,
                         Element* element)
    : list_element_(element),
      list_container_(list_container_impl),
      item_holder_map_(std::make_unique<list::ItemHolderMap>()),
      adapter_helper_(std::make_unique<AdapterHelper>()) {
  if (!list_element_ || !list_container_) {
    NLIST_LOGE(
        "[ListAdapter] error: list_element_ or list_container_ is nullptr");
  }
  adapter_helper_->SetDelegate(this);
}

void ListAdapter::OnErrorOccurred(lynx::base::LynxError error) {
  ElementManager* element_manager = list_element_->element_manager();
  if (element_manager) {
    element_manager->OnErrorOccurred(std::move(error));
  }
}

// Update data source for radon diff arch.
bool ListAdapter::UpdateDataSource(const lepus::Value& data_source) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListAdapter::UpdateDataSource");
  bool has_updated = false;
  if (data_source.IsObject() && adapter_helper_) {
    // Parse diff result.
    ForEachLepusValue(
        data_source, [this, &has_updated](const lepus::Value& key,
                                          const lepus::Value& value) {
          if (key.StdString() == list::kDiffResult) {
            has_updated = adapter_helper_->UpdateDiffResult(value);
          }
        });
    // Mark dirty based on index.
    MarkChildHolderDirty();
    // init list_container_info
    auto list_container_info = lepus::Dictionary::Create();
    // TODO(dingwang.wxx): Check whether the following traversal can be skipped
    // if the has_updated is not true. Update extra info.
    ForEachLepusValue(
        data_source, [this, list_container_info](const lepus::Value& key,
                                                 const lepus::Value& value) {
          const auto& key_str = key.StdString();
          if (key_str == list::kDataSourceEstimatedHeightPx) {
            adapter_helper_->UpdateEstimatedHeightsPx(value);
          } else if (key_str == list::kDataSourceEstimatedMainAxisSizePx) {
            adapter_helper_->UpdateEstimatedSizesPx(value);
          } else if (key_str == list::kDataSourceFullSpan) {
            adapter_helper_->UpdateFullSpans(value);
          } else if (key_str == list::kDataSourceStickyTop) {
            adapter_helper_->UpdateStickyTops(value);
            list_container_info->SetValue(
                BASE_STATIC_STRING(list::kDataSourceStickyTop), value);
          } else if (key_str == list::kDataSourceStickyBottom) {
            adapter_helper_->UpdateStickyBottoms(value);
            list_container_info->SetValue(
                BASE_STATIC_STRING(list::kDataSourceStickyBottom), value);
          } else if (key_str == list::kDataSourceItemKeys) {
            adapter_helper_->UpdateItemKeys(value);
            list_container_info->SetValue(
                BASE_STATIC_STRING(list::kDataSourceItemKeys), value);
          }
        });
    // flush list-container-info
    if (list_container_ && list_container_->element() &&
        list_container_->element_manager()) {
      auto* element = list_container_->element();
      if (element != nullptr && element->is_radon_element()) {
        // For FiberArch, no need to SetAttribute here,
        // we will call FiberElement::SetAttributeInternal in
        // ListElement::SetAttributeInternal.
        element->SetAttribute(BASE_STATIC_STRING(list::kListContainerInfo),
                              lepus::Value(list_container_info));
      }
    }
  }
  // For output list diff info before clear
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ListAdapter::UpdateDataSource.OutputDiffInfo",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  return has_updated;
}

// Update data source for fiber arch.
bool ListAdapter::UpdateFiberDataSource(const lepus::Value& data) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListAdapter::UpdateFiberDataSource");
  if (!data.IsTable()) {
    return false;
  }
  const auto& insert_action =
      data.GetProperty(BASE_STATIC_STRING(list::kFiberInsertAction));
  const auto& remove_action =
      data.GetProperty(BASE_STATIC_STRING(list::kFiberRemoveAction));
  const auto& update_action =
      data.GetProperty(BASE_STATIC_STRING(list::kFiberUpdateAction));
  // Firstly only generate insert / remove / update arrays.
  adapter_helper_->UpdateFiberRemoveAction(remove_action);
  adapter_helper_->UpdateFiberInsertAction(insert_action);
  adapter_helper_->UpdateFiberUpdateAction(update_action);
  // Mark dirty based on index.
  MarkChildHolderDirty();
  // Parse extra info from insert / remove / update actions.
  adapter_helper_->UpdateFiberRemoveAction(remove_action, false);
  adapter_helper_->UpdateFiberInsertAction(insert_action, false);
  adapter_helper_->UpdateFiberUpdateAction(update_action, false);
  // Update extra info.
  adapter_helper_->UpdateFiberExtraInfo();
  // For output list diff info before clear
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ListAdapter::UpdateFiberDataSource.OutputDiffInfo",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  return adapter_helper_->HasValidDiff();
}

void ListAdapter::UpdateListContainerDataSource(
    fml::RefPtr<lepus::Dictionary>& list_container_info) {
  // init list_container_info
  auto lepus_item_keys = lepus::CArray::Create();
  for (auto item_key : adapter_helper_->item_keys()) {
    lepus_item_keys->emplace_back(item_key);
  }
  list_container_info->SetValue(BASE_STATIC_STRING(list::kDataSourceItemKeys),
                                lepus_item_keys);

  auto lepus_stick_tops = lepus::CArray::Create();
  for (auto cur : adapter_helper_->sticky_tops()) {
    lepus_stick_tops->emplace_back(cur);
  }
  list_container_info->SetValue(BASE_STATIC_STRING(list::kDataSourceStickyTop),
                                lepus_stick_tops);

  auto lepus_stick_bottoms = lepus::CArray::Create();
  for (auto cur : adapter_helper_->sticky_bottoms()) {
    lepus_stick_bottoms->emplace_back(cur);
  }
  list_container_info->SetValue(
      BASE_STATIC_STRING(list::kDataSourceStickyBottom), lepus_stick_bottoms);
}

// Update the latest data source to the ItemHolder and add updated ItemHolders
// to children_ set in ChildrenHelper. If has new insertions, create ItemHolders
// and add them to the ItemHolder map.
void ListAdapter::UpdateItemHolderToLatest(
    ListChildrenHelper* list_children_helper) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListAdapter::UpdateItemHolderToLatest");
  if (!list_children_helper) {
    return;
  }

  const auto& children = list_children_helper->children();
  const auto& attached_children = list_children_helper->attached_children();
  const auto& last_binding_children =
      list_children_helper->last_binding_children();
  std::unordered_set<ItemHolder*> attached_children_set(
      attached_children.begin(), attached_children.end());
  std::unordered_set<ItemHolder*> last_binding_children_set(
      last_binding_children.begin(), last_binding_children.end());
  list_children_helper->ClearChildren();
  // Note: If has diff info, the attached children set needs to be rebuilt to
  // delete removed item holders.
  list_children_helper->ClearAttachedChildren();
  list_children_helper->ClearLastBindingChildren();
  ItemHolder* item_holder = nullptr;
  auto it = item_holder_map_->end();
  for (const auto& pair : adapter_helper_->item_key_map()) {
    const auto& item_key = pair.first;
    int new_index = pair.second;
    it = item_holder_map_->find(item_key);
    if (item_holder_map_->end() != it) {
      item_holder = it->second.get();
      // One component with item-key == 'x' removed from 5 and inserted to 10,
      // here the item_holder is marked removed. We reuse this item_holder for
      // index 10, so we should clear removed flag and mark dirty.
      if (IsRemoved(item_holder)) {
        OnItemHolderReInsert(item_holder);
      }
    } else {
      (*item_holder_map_)[item_key] =
          std::make_unique<ItemHolder>(new_index, item_key);
      item_holder = (*item_holder_map_)[item_key].get();
      OnItemHolderInserted(item_holder);
    }
    CheckSticky(item_holder, new_index);
    item_holder->SetIndex(new_index);
    item_holder->SetItemFullSpan(IsFullSpanAtIndex(new_index));
    item_holder->SetEstimatedSize(GetEstimatedSizeForIndex(new_index));
    list_children_helper->AddChild(children, item_holder);
    if (attached_children_set.find(item_holder) !=
        attached_children_set.end()) {
      // Add item holder to attached children.
      Element* component = GetListItemElement(item_holder);
      list_children_helper->AttachChild(item_holder, component);
    }
    if (last_binding_children_set.find(item_holder) !=
        last_binding_children_set.end()) {
      // Add item holder to last binding children.
      list_children_helper->AddChild(last_binding_children, item_holder);
    }
  }
}

// Mark all child ItemHolders's diff status.
void ListAdapter::MarkChildHolderDirty() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListAdapter::MarkChildHolderDirty");
  if (!adapter_helper_) {
    return;
  }
  ItemHolder* child_holder = nullptr;
  for (const auto& cur : adapter_helper_->removals()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderRemoved(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->move_to()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderMovedTo(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->move_from()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderMovedFrom(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->update_to()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderUpdateTo(child_holder);
    }
  }
  for (const auto& cur : adapter_helper_->update_from()) {
    child_holder = GetItemHolderForIndex(cur);
    if (child_holder) {
      OnItemHolderUpdateFrom(child_holder);
    }
  }
}

// Get the ItemHolder for the specified index.
ItemHolder* ListAdapter::GetItemHolderForIndex(int index) {
  if (adapter_helper_ && index >= 0 && index < GetDataCount()) {
    auto item_key = adapter_helper_->GetItemKeyForIndex(index);
    if (item_key &&
        item_holder_map_->end() != item_holder_map_->find(*item_key)) {
      return ((*item_holder_map_)[*item_key]).get();
    }
  }
  return nullptr;
}

// Get whether the ItemHolder is full span for the specified index.
bool ListAdapter::IsFullSpanAtIndex(int index) {
  return adapter_helper_ ? adapter_helper_->full_spans().end() !=
                               adapter_helper_->full_spans().find(index)
                         : false;
}

// Get estimated height for the specified index.
float ListAdapter::GetEstimatedSizeForIndex(int index) {
  float estimated_size = list::kInvalidDimensionSize;
  const float layouts_unit_per_px =
      (list_container_ && list_container_->element_manager())
          ? list_container_->element_manager()
                ->GetLynxEnvConfig()
                .LayoutsUnitPerPx()
          : 0.f;
  // Note: Taking into account compatibility with lower versions, developers
  // might set both estimated-main-axis-size-px and estimated-height-px,
  // therefore, using estimated-main-axis-size-px in priority.
  if (adapter_helper_ && index >= 0 &&
      index < static_cast<int>(adapter_helper_->estimated_sizes_px().size())) {
    estimated_size =
        adapter_helper_->estimated_sizes_px()[index] * layouts_unit_per_px;
    if (estimated_size > 0.f) {
      return estimated_size;
    }
  }
  if (adapter_helper_ && index >= 0 &&
      index <
          static_cast<int>(adapter_helper_->estimated_heights_px().size())) {
    estimated_size =
        adapter_helper_->estimated_heights_px()[index] * layouts_unit_per_px;
    if (estimated_size > 0.f) {
      return estimated_size;
    }
  }
  return list::kInvalidDimensionSize;
}

// Check whether the ItemHolder is sticky item with the specified index.
void ListAdapter::CheckSticky(ItemHolder* item_holder, int32_t index) {
  if (!adapter_helper_) {
    return;
  }
  bool sticky_top = false;
  if (std::find(adapter_helper_->sticky_tops().begin(),
                adapter_helper_->sticky_tops().end(),
                (int32_t)index) != adapter_helper_->sticky_tops().end()) {
    sticky_top = true;
  }
  bool sticky_bottom = false;
  if (std::find(adapter_helper_->sticky_bottoms().begin(),
                adapter_helper_->sticky_bottoms().end(),
                (int32_t)index) != adapter_helper_->sticky_bottoms().end()) {
    sticky_bottom = true;
  }
  item_holder->SetSticky(sticky_top, sticky_bottom);
}

int64_t ListAdapter::GenerateOperationId() const {
  static int32_t base_operation_id = 0;
  return (static_cast<int64_t>(list_element_->impl_id()) << 32) +
         base_operation_id++;
}

void ListAdapter::RecycleAllItemHolders() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListAdapter::RecycleAllItemHolders");
  for (auto it = item_holder_map_->begin(); it != item_holder_map_->end();) {
    const auto& item_holder = it->second;
    if (item_holder) {
      RecycleItemHolder(item_holder.get());
    }
    ++it;
  }
}

void ListAdapter::RecycleRemovedItemHolders() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListAdapter::RecycleRemovedItemHolders");
  for (auto it = item_holder_map_->begin(); it != item_holder_map_->end();) {
    const auto& item_holder = it->second;
    if (item_holder && IsRemoved(item_holder.get())) {
      RecycleItemHolder(item_holder.get());
      item_holder_map_->erase(it++);
    } else {
      ++it;
    }
  }
}

void ListAdapter::UpdateLayoutInfoToItemHolder(Element* list_item,
                                               ItemHolder* item_holder) {
  if (list_item && item_holder && IsFinishedBinding(item_holder) &&
      GetListItemElement(item_holder) == list_item) {
    item_holder->UpdateLayoutFromElement(list_item);
  }
}

#if ENABLE_TRACE_PERFETTO
void ListAdapter::UpdateTraceDebugInfo(TraceEvent* event) {
  // item-key
  auto* item_keys = event->add_debug_annotations();
  item_keys->set_name("item-keys");
  std::string item_keys_str = "";
  int index = 0;
  for (const auto& item_key : adapter_helper_->item_keys()) {
    item_keys_str += "(" + std::to_string(index++) + ") " + item_key + "\n";
  }
  item_keys->set_string_value(item_keys_str);

  bool has_update = false;
  // update-from
  auto* update_from = event->add_debug_annotations();
  update_from->set_name("update-from");
  std::string update_from_str = "";
  for (const auto& index : adapter_helper_->update_from()) {
    has_update = true;
    update_from_str += std::to_string(index) + "\n";
  }
  update_from->set_string_value(update_from_str);

  // update-to
  auto* update_to = event->add_debug_annotations();
  update_to->set_name("update-to");
  std::string update_to_str = "";
  for (const auto& index : adapter_helper_->update_to()) {
    has_update = true;
    update_to_str += std::to_string(index) + "\n";
  }
  update_to->set_string_value(update_to_str);

  // insert
  auto* insert = event->add_debug_annotations();
  insert->set_name("insert");
  std::string insert_str = "";
  for (const auto& index : adapter_helper_->insertions()) {
    has_update = true;
    insert_str += std::to_string(index) + "\n";
  }
  insert->set_string_value(insert_str);

  // remove
  auto* remove = event->add_debug_annotations();
  remove->set_name("remove");
  std::string remove_str = "";
  for (const auto& index : adapter_helper_->removals()) {
    has_update = true;
    remove_str += std::to_string(index) + "\n";
  }
  remove->set_string_value(remove_str);

  // has update
  auto* has_update_annotations = event->add_debug_annotations();
  has_update_annotations->set_name("has_update");
  has_update_annotations->set_string_value(std::to_string(has_update));

  // sticky top
  auto* sticky_top = event->add_debug_annotations();
  sticky_top->set_name("sticky-top");
  std::string sticky_top_str = "";
  for (const auto& index : adapter_helper_->sticky_tops()) {
    sticky_top_str += std::to_string(index) + "\n";
  }
  sticky_top->set_string_value(sticky_top_str);

  // sticky bottom
  auto* sticky_bottom = event->add_debug_annotations();
  sticky_bottom->set_name("sticky-bottom");
  std::string sticky_bottom_str = "";
  for (const auto& index : adapter_helper_->sticky_bottoms()) {
    sticky_bottom_str += std::to_string(index) + "\n";
  }
  sticky_bottom->set_string_value(sticky_bottom_str);

  // full span
  auto* full_span = event->add_debug_annotations();
  full_span->set_name("full-span");
  std::string full_span_str = "";
  for (const auto& index : adapter_helper_->full_spans()) {
    full_span_str += std::to_string(index) + "\n";
  }
  full_span->set_string_value(full_span_str);

  // estimated_heights_px
  auto* estimated_heights_px = event->add_debug_annotations();
  estimated_heights_px->set_name("estimated-heights-px");
  std::string estimated_heights_px_str = "";
  for (const auto& value : adapter_helper_->estimated_heights_px()) {
    estimated_heights_px_str += std::to_string(value) + "\n";
  }
  estimated_heights_px->set_string_value(estimated_heights_px_str);

  // estimated_sizes_px
  auto* estimated_sizes_px = event->add_debug_annotations();
  estimated_sizes_px->set_name("estimated-sizes-px");
  std::string estimated_sizes_px_str = "";
  for (const auto& value : adapter_helper_->estimated_sizes_px()) {
    estimated_sizes_px_str += std::to_string(value) + "\n";
  }
  estimated_sizes_px->set_string_value(estimated_sizes_px_str);
}
#endif

}  // namespace tasm
}  // namespace lynx
