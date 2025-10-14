// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/lynx_list_data.h"

#include <algorithm>
#include <string>
#include <utility>

#include "clay/fml/logging.h"

namespace clay {

LynxListData::LynxListData(int count) : count_(count) {
  view_types_.reserve(count_);
}

LynxListData::~LynxListData() = default;

void LynxListData::PushViewType(const char* name) {
  FML_DCHECK(name);
  FML_DCHECK((int)view_types_.size() < count_);
  view_types_.push_back(GetViewTypeIndex(name));
}

void LynxListData::SetFullSpan(const uint32_t* indices, int size) {
  full_span_.clear();
  full_span_.insert(full_span_.end(), indices, indices + size);
  std::sort(full_span_.begin(), full_span_.end());
}

void LynxListData::SetStickyTop(const uint32_t* indices, int size) {
  sticky_top_.clear();
  sticky_top_.insert(sticky_top_.end(), indices, indices + size);
  std::sort(sticky_top_.begin(), sticky_top_.end());
}

void LynxListData::SetStickyBottom(const uint32_t* indices, int size) {
  sticky_bottom_.clear();
  sticky_bottom_.insert(sticky_bottom_.end(), indices, indices + size);
  std::sort(sticky_bottom_.begin(), sticky_bottom_.end());
}

void LynxListData::SetInsertions(const int32_t* indices, int size) {
  insertions_.clear();
  insertions_.insert(insertions_.end(), indices, indices + size);
}

void LynxListData::SetRemovals(const int32_t* indices, int size) {
  removals_.clear();
  removals_.insert(removals_.end(), indices, indices + size);
}

void LynxListData::SetUpdateFrom(const int32_t* indices, int size) {
  update_from_.clear();
  update_from_.insert(update_from_.end(), indices, indices + size);
}

void LynxListData::SetUpdateTo(const int32_t* indices, int size) {
  update_to_.clear();
  update_to_.insert(update_to_.end(), indices, indices + size);
}

void LynxListData::SetMoveFrom(const int32_t* indices, int size) {
  move_from_.clear();
  move_from_.insert(move_from_.end(), indices, indices + size);
}

void LynxListData::SetMoveTo(const int32_t* indices, int size) {
  move_to_.clear();
  move_to_.insert(move_to_.end(), indices, indices + size);
}

void LynxListData::InsertViewType(int index, const std::string& type_name) {
  if (static_cast<size_t>(index) > view_types_.size()) {
    FML_DCHECK(false)
        << "[LIST][LynxListData::InsertViewType] index out of bounds (" << index
        << "/" << view_types_.size() << ")";
    return;
  }
  view_types_.insert(view_types_.begin() + index, GetViewTypeIndex(type_name));
}

void LynxListData::SetViewType(int index, const std::string& type_name) {
  if (static_cast<size_t>(index) >= view_types_.size()) {
    FML_DCHECK(false)
        << "[LIST][LynxListData::SetViewType] index out of bounds (" << index
        << "/" << view_types_.size() << ")";
    return;
  }
  view_types_[index] = GetViewTypeIndex(type_name);
}

void LynxListData::RemoveViewType(int index) {
  if (static_cast<int>(view_types_.size()) > index) {
    view_types_.erase(view_types_.begin() + index);
  } else {
    FML_DCHECK(false) << "[LIST] The index(" << index
                      << ") to be removed does not exist";
  }
}

int LynxListData::GetViewTypeIndex(const std::string& type_name) {
  int name_index;

  // Find if the name has been registered or not
  auto itr = registered_.find(type_name);
  if (itr != registered_.end()) {
    name_index = itr->second;
  } else {
    name_index = view_type_names_.size();
    view_type_names_.emplace_back(type_name);
    registered_.emplace(type_name, name_index);
  }

  return name_index;
}

void LynxListData::DispatchNoDiffActions(ListNoDiffInfo* info) {
  for (int i = info->remove_actions.size() - 1; i >= 0; i--) {
    int position = info->remove_actions[i];
    RemoveViewType(position);
    fiber_full_span_.erase(fiber_full_span_.begin() + position);
    fiber_sticky_top_.erase(fiber_sticky_top_.begin() + position);
    fiber_sticky_bottom_.erase(fiber_sticky_bottom_.begin() + position);
  }

  for (const auto& action : info->insert_actions) {
    int position = action.position;
    InsertViewType(position, action.type);
    fiber_full_span_.insert(fiber_full_span_.begin() + position,
                            action.full_span);
    fiber_sticky_top_.insert(fiber_sticky_top_.begin() + position,
                             action.sticky_top);
    fiber_sticky_bottom_.insert(fiber_sticky_bottom_.begin() + position,
                                action.sticky_bottom);
  }

  for (const auto& action : info->update_actions) {
    SetViewType(action.from, action.type);
    fiber_full_span_[action.from] = action.full_span;
    fiber_sticky_top_[action.from] = action.sticky_top;
    fiber_sticky_bottom_[action.from] = action.sticky_bottom;
  }
}

void LynxListData::TransformExtraData() {
  full_span_.clear();
  for (size_t i = 0; i < fiber_full_span_.size(); i++) {
    if (fiber_full_span_[i]) {
      full_span_.push_back(i);
    }
  }

  sticky_top_.clear();
  for (size_t i = 0; i < fiber_sticky_top_.size(); i++) {
    if (fiber_sticky_top_[i]) {
      sticky_top_.push_back(i);
    }
  }

  sticky_bottom_.clear();
  for (size_t i = 0; i < fiber_sticky_bottom_.size(); i++) {
    if (fiber_sticky_bottom_[i]) {
      sticky_bottom_.push_back(i);
    }
  }
}

}  // namespace clay
