// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_view.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager.h"
#include "clay/ui/component/list/list_layout_manager_linear.h"
#include "clay/ui/component/list/lynx_list_adapter.h"
#include "clay/ui/component/page_view.h"

namespace clay {

namespace {

const clay::Value* FindMapItem(const clay::Value::Map& map,
                               const std::string& key) {
  const auto it = map.find(key);
  if (it != map.end()) {
    return &it->second;
  }
  return nullptr;
}

inline int SafeGetInt(const clay::Value* value, int default_value = 0) {
  if (!value) {
    return default_value;
  }
  double result = default_value;
  attribute_utils::TryGetNum(*value, result, default_value);
  return result;
}

inline bool SafeGetBool(const clay::Value* value, bool default_value = false) {
  if (!value) {
    return default_value;
  }
  return attribute_utils::GetBool(*value, default_value);
}

inline std::string SafeGetString(const clay::Value* value,
                                 std::string default_value = "") {
  if (!value) {
    return default_value;
  }
  std::string result = default_value;
  attribute_utils::TryGetString(*value, result, default_value);
  return result;
}

}  // namespace

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
  if (!HasEvent(event_attr::kEventListLayoutComplete)) {
    return;
  }
  Value::Array cells;
  for (int i = 0; i < lynx_adapter_->GetItemCount(); i++) {
    cells.emplace_back(lynx_adapter_->GetItemTypeName(i));
  }
  page_view_->SendEvent(callback_id_, event_attr::kEventListLayoutComplete,
                        {"timestamp", "cells"},
                        fml::TimePoint::Now().ToEpochDelta().ToMilliseconds(),
                        cells);
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
    SetNoDiffInfo(value);
    is_no_diff_mode_ = true;
  } else {
    BaseListView::SetAttribute(attr_c, value);
  }
}
void ListView::SetNoDiffInfo(const clay::Value& value) {
  if (!value.IsMap()) {
    FML_DLOG(ERROR) << "the value of 'update-list-info' must be a map";
    return;
  }

  auto info = std::make_unique<ListNoDiffInfo>();
  const auto& map = value.GetMap();
  auto update_actions = FindMapItem(map, "updateAction");
  auto insert_actions = FindMapItem(map, "insertAction");
  auto remove_actions = FindMapItem(map, "removeAction");

  if (update_actions && update_actions->IsArray()) {
    const auto& arr = update_actions->GetArray();
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
    for (size_t i = 0; i < arr.size(); i++) {
      info->remove_actions.push_back(SafeGetInt(&arr[i]));
    }
  }

  no_diff_info_ = std::move(info);
}

}  // namespace clay
