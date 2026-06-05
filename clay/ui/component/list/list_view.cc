// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager.h"
#include "clay/ui/component/list/list_layout_manager_linear.h"
#include "clay/ui/component/list/lynx_list_adapter.h"
#include "clay/ui/component/page_view.h"

namespace clay {

ListView::ListView(int id, PageView* page_view) : ListView(id, id, page_view) {}

ListView::ListView(int id, int callback_id, PageView* page_view)
    : BaseListView(id, callback_id, "list", page_view) {
  InitAdapter();

  SetLayoutManager(
      ListLayoutManager::Create(layout_manager_type_, layout_span_count_));
}

ListView::~ListView() { SetAdapter(nullptr); }

void ListView::AddChild(BaseView* child, int index) {
  BaseListView::AddChild(child, index);
  if (waiting_for_child_.has_value()) {
    FML_DCHECK(waiting_for_child_.value() != nullptr);
    (*waiting_for_child_)->SetView(child);
    waiting_for_child_.reset();
  }
}

void ListView::DidUpdateAttributes() {
  BaseView::DidUpdateAttributes();
  if (is_no_diff_mode_) {
    lynx_adapter_->UpdateActions(no_diff_info_.get());
    no_diff_info_.reset();
  } else {
    lynx_adapter_->UpdateData();
  }
}

std::unique_ptr<LynxListData> ListView::GetListData() {
  auto delegate = page_view_->GetUIComponentDelegate();
  if (!delegate) {
    return nullptr;
  }
  LynxListData* raw_ptr = delegate->OnListGetData(callback_id_);
  return std::unique_ptr<LynxListData>(raw_ptr);
}

void ListView::UpdateChildPosition(ListItemViewHolder* item, int position) {
  TRACE_EVENT("LIST", "ListView::UpdateChildPosition");
  auto delegate = page_view_->GetUIComponentDelegate();
  if (!delegate) {
    return;
  }
  delegate->OnUpdateChild(callback_id_, item->GetView()->id(), position,
                          GenerateOperationId());
}

BaseView* ListView::ObtainChild(ListItemViewHolder* item, int position) {
  TRACE_EVENT("LIST", "ListView::ObtainChild");
  auto delegate = page_view_->GetUIComponentDelegate();
  if (!delegate) {
    return nullptr;
  }
  int child_id =
      delegate->OnObtainChild(callback_id_, position, GenerateOperationId());
  return page_view_->FindViewByViewId(child_id);
}

void ListView::RecycleChild(BaseView* child) {
  TRACE_EVENT("LIST", "ListView::RecycleChild");
  auto delegate = page_view_->GetUIComponentDelegate();
  if (!delegate) {
    return;
  }
  delegate->OnRecycleChild(callback_id_, child->id());
}

BaseView* ListView::HandleCreateView(ListItemViewHolder* item) {
  auto delegate = page_view_->GetUIComponentDelegate();
  if (!delegate) {
    return nullptr;
  }
  waiting_for_child_ = item;
  const int position = item->GetPosition();
  delegate->OnCreateAddChild(callback_id_, position, 0);
  FML_DCHECK(!waiting_for_child_.has_value());
  BaseView* temp = item->GetView();

  return temp;
}

void ListView::ProcessAdapterUpdates() {
  BaseListView::ProcessAdapterUpdates();
  lynx_adapter_->MarkUpdatesConsumed();
}

void ListView::OnLayoutComplete() {
  if (!HasEvent(event_attr::kEventLayoutComplete)) {
    return;
  }
  Value::Array cells;
  for (int i = 0; i < lynx_adapter_->GetItemCount(); i++) {
    cells.emplace_back(lynx_adapter_->GetItemTypeName(i));
  }
  page_view_->SendEvent(
      callback_id_, event_attr::kEventLayoutComplete, {"timestamp", "cells"},
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds(), cells);
}

void ListView::InitAdapter() {
  lynx_adapter_ = std::make_unique<LynxListAdapter>(this);
  SetAdapter(lynx_adapter_.get());
}

int64_t ListView::GenerateOperationId() {
  return (static_cast<int64_t>(callback_id_) << 32) + op_counter_++;
}

void ListView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kUpdateListInfo) {
    is_no_diff_mode_ = SetNoDiffInfo(value);
  } else {
    BaseListView::SetAttribute(attr_c, value);
  }
}

bool ListView::SetNoDiffInfo(const clay::Value& value) {
  if (!value.IsMap()) {
    FML_DLOG(ERROR) << "the value of 'update-list-info' must be a map";
    no_diff_info_.reset();
    return false;
  }
  auto info = std::make_unique<ListNoDiffInfo>();
  const auto& map = value.GetMap();
  auto update_actions = FindMapItem(map, "updateAction");
  auto insert_actions = FindMapItem(map, "insertAction");
  auto remove_actions = FindMapItem(map, "removeAction");
  auto is_valid_action_array = [](const clay::Value* v) {
    return v == nullptr || v->IsArray();
  };
  if (!is_valid_action_array(update_actions) ||
      !is_valid_action_array(insert_actions) ||
      !is_valid_action_array(remove_actions)) {
    FML_DLOG(ERROR) << "invalid 'update-list-info' action array type";
    no_diff_info_.reset();
    return false;
  }
  bool has_action = false;
  if (update_actions && update_actions->IsArray()) {
    const auto& arr = update_actions->GetArray();
    has_action = has_action || !arr.empty();
    for (size_t i = 0; i < arr.size(); i++) {
      const auto& map = arr[i].GetMap();
      ListNoDiffInfo::UpdateAction action;
      action.from = SafeGetInt(FindMapItem(map, "from"));
      action.to = SafeGetInt(FindMapItem(map, "to"));
      action.item_key = SafeGetString(FindMapItem(map, "item-key"));
      action.type = SafeGetString(FindMapItem(map, "type"));
      action.full_span = SafeGetBool(FindMapItem(map, "full-span"));
      action.sticky_top = SafeGetBool(FindMapItem(map, "sticky-top"));
      action.sticky_bottom = SafeGetBool(FindMapItem(map, "sticky-bottom"));
      action.estimated_height_px =
          SafeGetInt(FindMapItem(map, "estimated-height-px"), -1);
      action.is_flush = SafeGetBool(FindMapItem(map, "flush"));
      info->update_actions.push_back(action);
    }
  }
  if (insert_actions && insert_actions->IsArray()) {
    const auto& arr = insert_actions->GetArray();
    has_action = has_action || !arr.empty();
    for (size_t i = 0; i < arr.size(); i++) {
      const auto& map = arr[i].GetMap();
      ListNoDiffInfo::InsertAction action;
      action.position = SafeGetInt(FindMapItem(map, "position"));
      action.item_key = SafeGetString(FindMapItem(map, "item-key"));
      action.type = SafeGetString(FindMapItem(map, "type"));
      action.full_span = SafeGetBool(FindMapItem(map, "full-span"));
      action.sticky_top = SafeGetBool(FindMapItem(map, "sticky-top"));
      action.sticky_bottom = SafeGetBool(FindMapItem(map, "sticky-bottom"));
      action.estimated_height_px =
          SafeGetInt(FindMapItem(map, "estimated-height-px"), -1);
      info->insert_actions.push_back(action);
    }
  }
  if (remove_actions && remove_actions->IsArray()) {
    const auto& arr = remove_actions->GetArray();
    has_action = has_action || !arr.empty();
    for (size_t i = 0; i < arr.size(); i++) {
      info->remove_actions.push_back(SafeGetInt(&arr[i]));
    }
  }
  if (!has_action) {
    no_diff_info_.reset();
    return false;
  }
  no_diff_info_ = std::move(info);
  return true;
}

}  // namespace clay
