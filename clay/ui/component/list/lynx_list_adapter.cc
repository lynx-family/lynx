// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/lynx_list_adapter.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/ui/component/list/list_view.h"
#include "clay/ui/component/list/lynx_list_item_view_holder.h"
#include "clay/ui/component/list/macros.h"

namespace clay {

namespace {

template <typename T>
void RemoveVectorValue(std::vector<T>* vec, T value) {
  for (auto it = vec->begin(); it != vec->end(); it++) {
    if (*it == value) {
      vec->erase(it);
      return;
    }
  }
}

}  // namespace

LynxListAdapter::LynxListAdapter(ListView* list_view)
    : ListAdapter(list_view) {}

LynxListAdapter::~LynxListAdapter() = default;

int LynxListAdapter::GetItemCount() const {
  if (list_data_) {
    return list_data_->GetViewTypes().size();
  }
  return 0;
}

ListAdapter::TypeId LynxListAdapter::GetItemViewType(int position) {
  FML_DCHECK(list_data_);
  const size_t index = list_data_->GetViewTypes()[position];
  FML_DCHECK(index < list_data_->GetViewTypeNames().size());
  const std::string& type_name = list_data_->GetViewTypeNames()[index];
  return type_names_[type_name];
}

std::string LynxListAdapter::GetItemTypeName(int position) const {
  FML_DCHECK(list_data_);
  const size_t index = list_data_->GetViewTypes()[position];
  FML_DCHECK(index < list_data_->GetViewTypeNames().size());
  return list_data_->GetViewTypeNames()[index];
}

void LynxListAdapter::UpdateData() {
  bool full_flush = false;
  if (list_data_ == nullptr) {
    full_flush = true;
  } else if (list_data_->GetViewTypes().size() ==
             list_data_->GetFullSpan().size()) {
    full_flush = true;
  }

  {
    TRACE_EVENT("clay", "RK::GetListData");
    list_data_ = GetListView()->GetListData();
  }

  FML_DCHECK(list_data_->GetUpdateFrom().size() ==
             list_data_->GetUpdateTo().size());
  FML_DCHECK(list_data_->GetMoveFrom().size() ==
             list_data_->GetMoveTo().size());

  UpdateTypeNames();

  bool diffable = list_data_ && list_data_->GetDiffable();
  if (!full_flush && diffable && updates_consumed_) {
    DispatchDataUpdate();
  } else {
    NotifyDataSetChanged();
  }

#if (DEBUG_LIST)
  auto print_vector = [](const auto& vec) -> std::string {
    std::stringstream ss;
    for (auto element : vec) {
      ss << element << ",";
    }
    return ss.str();
  };
  LIST_LOG << "After fetching list data from lynx\n";
  LIST_LOG << "ViewTypes:" << print_vector(list_data_->GetViewTypes());
  LIST_LOG << "ViewTypeNames:" << print_vector(list_data_->GetViewTypeNames());
  LIST_LOG << "FullSpan:" << print_vector(list_data_->GetFullSpan());
  LIST_LOG << "Insertions:" << print_vector(list_data_->GetInsertions());
  LIST_LOG << "Removals:" << print_vector(list_data_->GetRemovals());
  LIST_LOG << "UpdateFrom:" << print_vector(list_data_->GetUpdateFrom());
  LIST_LOG << "UpdateTo:" << print_vector(list_data_->GetUpdateTo());
  LIST_LOG << "MoveFrom:" << print_vector(list_data_->GetMoveFrom());
  LIST_LOG << "MoveTo:" << print_vector(list_data_->GetMoveTo());
#endif
}

void LynxListAdapter::UpdateActions(ListNoDiffInfo* info) {
  if (!info || (info->update_actions.empty() && info->insert_actions.empty() &&
                info->remove_actions.empty())) {
    return;
  }
  if (!list_data_) {
    list_data_ = std::make_unique<LynxListData>(0);
  }
  list_data_->SetNewArch(true);

  list_data_->DispatchNoDiffActions(info);
  list_data_->TransformExtraData();
  FlushNoDiffActions(info);
}

void LynxListAdapter::PrintViewHolders() {
#if (DEBUG_LIST)
  int i = 0;
  std::stringstream ss;
  for (const auto& child : view_holders_) {
    ss << "\n[" << i << "]" << child->ToString();
    ++i;
  }
  LIST_LOG << "ViewHolders:" << ss.str();
#endif
}

ListItemViewHolder* LynxListAdapter::OnCreateListItem(TypeId type) {
  auto item = std::make_unique<LynxListItemViewHolder>(type);
  view_holders_.emplace_back(std::move(item));
  return view_holders_.back().get();
}

void LynxListAdapter::OnBindListItem(ListItemViewHolder* item,
                                     int prev_position, int position,
                                     bool newly_created) {
  item->SetFullSpan(IsItemFullSpan(position));
  item->SetStickyType(IsItemStickyTop(position) ? ListItemStickyType::kTop
                      : IsItemStickyBottom(position)
                          ? ListItemStickyType::kBottom
                          : ListItemStickyType::kNone);

  if (IsNewArch()) {
    OnBindListItemOnNewArch(item, position);
    return;
  }

  if (newly_created) {
    return;
  }

  const Payloads& payloads = item->GetPayloads();
  if (payloads.size() == 0) {
    LIST_LOG << "item type:" << item->GetItemViewType()
             << " view_id:" << item->GetView()->id()
             << " move from:" << prev_position << " to:" << position;
    GetListView()->UpdateChildPosition(item, position);
  } else {
    const auto* lynx_payload =
        static_cast<LynxListPayload*>(payloads.back().get());
    LIST_LOG << "item type:" << item->GetItemViewType()
             << " view_id:" << item->GetView()->id()
             << " move from:" << prev_position << " to:" << position
             << " payload:" << lynx_payload->update_to_position;
    GetListView()->UpdateChildPosition(item, lynx_payload->update_to_position);
  }
}

void LynxListAdapter::OnBindListItemOnNewArch(ListItemViewHolder* item,
                                              int position) {
  auto child = GetListView()->ObtainChild(item, position);
  if (child != item->GetView()) {
    if (child->Parent()) {
      child->Parent()->BaseView::RemoveChild(child);
    }
    // recycle item
    if (item->GetView()) {
      GetListView()->RecycleChild(item->GetView());
      item->SetView(nullptr);
    } else {
      // Set the initial visibility to false because the just created item is
      // not  attached yet.
      // We don't call `SetViewVisible()` here because we don't want to send
      // NodeDisappear() notification for a newly created item.
      child->SetVisible(false);
    }
    item->SetView(child);
  }
}

void LynxListAdapter::OnDeleteListItem(ListItemViewHolder* view_holder) {
  if (auto view = view_holder->GetView()) {
    view->DestroyAllChildren();
    view->Destroy();
    delete view;
    view_holder->SetView(nullptr);
  }
}

void LynxListAdapter::OnRecycleItem(ListItemViewHolder* item) {
  ListAdapter::OnRecycleItem(item);
  // If the item is prefetched, we might use it in the future.
  // So we should not recycle it(another word, should not recycle its dom node,
  // just let it be).
  if (IsNewArch() && !item->IsPrefetch()) {
    GetListView()->RecycleChild(item->GetView());
    item->SetView(nullptr);
  }
}

void LynxListAdapter::UpdateTypeNames() {
  if (list_data_) {
    for (const std::string& type_name : list_data_->GetViewTypeNames()) {
      if (type_names_.find(type_name) == type_names_.end()) {
        type_names_[type_name] = type_names_.size();
      }
    }
  }
}

ListView* LynxListAdapter::GetListView() {
  return static_cast<ListView*>(list_view_);
}

void LynxListAdapter::DispatchDataUpdate() {
  FML_DCHECK(list_data_);

  if (!list_data_->GetRemovals().empty() ||
      !list_data_->GetInsertions().empty() ||
      !list_data_->GetUpdateFrom().empty() ||
      !list_data_->GetUpdateTo().empty() ||
      !list_data_->GetMoveFrom().empty() || !list_data_->GetMoveTo().empty()) {
    updates_consumed_ = false;
  }

  for (size_t i = 0; i < list_data_->GetUpdateFrom().size(); ++i) {
    NotifyItemRangeChanged(
        list_data_->GetUpdateFrom()[i], 1,
        std::make_unique<LynxListPayload>(list_data_->GetUpdateTo()[i]));
  }

  for (size_t i = 0; i < list_data_->GetMoveFrom().size(); ++i) {
    NotifyItemMoved(list_data_->GetMoveFrom()[i], list_data_->GetMoveTo()[i]);
  }

  for (int i = list_data_->GetRemovals().size() - 1; i >= 0; --i) {
    NotifyItemRangeRemoved(list_data_->GetRemovals()[i], 1);
  }

  for (int i : list_data_->GetInsertions()) {
    NotifyItemRangeInserted(i, 1);
  }
}

bool LynxListAdapter::IsItemFullSpan(int position) const {
  if (!list_data_) {
    return false;
  }

  const auto& full_spans = list_data_->GetFullSpan();
  return std::binary_search(full_spans.begin(), full_spans.end(), position);
}

bool LynxListAdapter::IsItemStickyTop(int position) const {
  if (!list_data_) {
    return false;
  }

  const auto& items = list_data_->GetStickyTop();
  return std::binary_search(items.begin(), items.end(), position);
}

bool LynxListAdapter::IsItemStickyBottom(int position) const {
  if (!list_data_) {
    return false;
  }

  const auto& items = list_data_->GetStickyBottom();
  return std::binary_search(items.begin(), items.end(), position);
}

bool LynxListAdapter::IsNewArch() const {
  return list_data_ && list_data_->GetNewArch();
}

void LynxListAdapter::MarkUpdatesConsumed() { updates_consumed_ = true; }

void LynxListAdapter::FlushNoDiffActions(ListNoDiffInfo* info) {
  for (auto position : info->remove_actions) {
    NotifyItemRangeRemoved(position, 1);
  }
  for (const auto& action : info->insert_actions) {
    NotifyItemRangeInserted(action.position, 1);
  }
  for (const auto& action : info->update_actions) {
    if (action.is_flush) {
      NotifyItemRangeChanged(action.from, 1,
                             std::make_unique<LynxListPayload>(action.to));
    }
  }
}

}  // namespace clay
