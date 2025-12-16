// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_adapter_helper.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace list {

std::string AdapterHelper::DiffResult::ToString() const {
  std::ostringstream oss;
  oss << "DiffResult: item_keys:[";
  for (const auto& item_key : item_keys_) {
    oss << item_key << ",";
  }
  oss << "],";
  auto diff_action_to_string = [&oss](const std::string& key,
                                      const std::vector<int32_t>& array) {
    oss << key << ":[";
    for (int32_t index : array) {
      oss << index << ",";
    }
    oss << "],";
  };
  diff_action_to_string(kRadonDataInsertions, insertions_);
  diff_action_to_string(kRadonDataRemovals, removals_);
  diff_action_to_string(kRadonDataUpdateFrom, update_from_);
  diff_action_to_string(kRadonDataUpdateTo, update_to_);
  diff_action_to_string(kRadonDataMoveFrom, move_from_);
  diff_action_to_string(kRadonDataMoveTo, move_to_);
  return oss.str();
}

//  update "diff-result" info  on radon_diff architecture
bool AdapterHelper::UpdateRadonDiffResult(const pub::Value& diff_result) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_DIFF_RESULT);
  bool has_update = false;
  if (diff_result.IsMap()) {
    diff_result.ForeachMap(
        [this, &has_update](const pub::Value& key, const pub::Value& value) {
          if (key.IsString()) {
            const std::string& key_str = key.str();
            if (key_str == kRadonDataInsertions) {
              UpdateInsertions(value);
              has_update = true;
            } else if (key_str == kRadonDataRemovals) {
              UpdateRemovals(value);
              has_update = true;
            } else if (key_str == kRadonDataUpdateFrom) {
              UpdateUpdateFrom(value);
              has_update = true;
            } else if (key_str == kRadonDataUpdateTo) {
              UpdateUpdateTo(value);
              has_update = true;
            } else if (key_str == kRadonDataMoveFrom) {
              UpdateMoveFrom(value);
              has_update = true;
            } else if (key_str == kRadonDataMoveTo) {
              UpdateMoveTo(value);
              has_update = true;
            }
          }
        });
  }
  return has_update;
}

void AdapterHelper::UpdateInsertions(const pub::Value& diff_insertions) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_INSERTIONS);
  diff_result_.insertions_.clear();
  if (diff_insertions.IsArray()) {
    diff_insertions.ForeachArray(
        [this](int64_t index, const pub::Value& value) {
          if (value.IsInt32() && value.Int32() >= 0) {
            diff_result_.insertions_.emplace_back(value.Int32());
          }
        });
  }
}

void AdapterHelper::UpdateRemovals(const pub::Value& diff_removals) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_REMOVALS);
  diff_result_.removals_.clear();
  if (diff_removals.IsArray()) {
    diff_removals.ForeachArray([this](int64_t index, const pub::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        diff_result_.removals_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateUpdateFrom(const pub::Value& diff_update_from) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_FROM);
  diff_result_.update_from_.clear();
  if (diff_update_from.IsArray()) {
    diff_update_from.ForeachArray(
        [this](int64_t index, const pub::Value& value) {
          if (value.IsInt32() && value.Int32() >= 0) {
            diff_result_.update_from_.emplace_back(value.Int32());
          }
        });
  }
}

void AdapterHelper::UpdateUpdateTo(const pub::Value& diff_update_to) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_TO);
  diff_result_.update_to_.clear();
  if (diff_update_to.IsArray()) {
    diff_update_to.ForeachArray([this](int64_t index, const pub::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        diff_result_.update_to_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateMoveTo(const pub::Value& diff_move_to) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_MOVE_TO);
  diff_result_.move_to_.clear();
  if (diff_move_to.IsArray()) {
    diff_move_to.ForeachArray([this](int64_t index, const pub::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        diff_result_.move_to_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateMoveFrom(const pub::Value& diff_move_from) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_MOVE_FROM);
  diff_result_.move_from_.clear();
  if (diff_move_from.IsArray()) {
    diff_move_from.ForeachArray([this](int64_t index, const pub::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        diff_result_.move_from_.emplace_back(value.Int32());
      }
    });
  }
}

// update "item-key" info on radon architecture
void AdapterHelper::UpdateItemKeys(const pub::Value& item_keys_value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_ITEM_KEYS);
  auto& item_keys = diff_result_.item_keys_;
  item_keys.clear();
  item_key_map_.clear();
  bool has_illegal_item_key = false;
  bool has_duplicated_item_key = false;
  if (item_keys_value.IsArray()) {
    item_keys_value.ForeachArray(
        [this, &has_illegal_item_key, &has_duplicated_item_key, &item_keys](
            int64_t index, const pub::Value& value) {
          if (value.IsString()) {
            const std::string& item_key = value.str();
            has_duplicated_item_key =
                has_duplicated_item_key ||
                item_key_map_.find(item_key) != item_key_map_.end();
            item_key_map_[item_key] = static_cast<int>(item_keys.size());
            item_keys.emplace_back(item_key);
          } else {
            has_illegal_item_key = true;
          }
        });
  }
  if (has_illegal_item_key && delegate_) {
    std::string error_msg = "Error for illegal list item-key.";
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_ILLEGAL_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
  if (has_duplicated_item_key && delegate_) {
    std::string error_msg = "Error for duplicated list item-key.";
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_DUPLICATE_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
}

// update "estimated-height-px" info on radon architecture
void AdapterHelper::UpdateEstimatedHeightsPx(
    const pub::Value& estimated_heights_px) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_ESTIMATED_HEIGHT);
  estimated_heights_px_.clear();
  if (estimated_heights_px.IsArray()) {
    estimated_heights_px.ForeachArray(
        [this](int64_t index, const pub::Value& value) {
          if (value.IsInt32()) {
            // Note: In radon arch, if not set estimated_heights_px, the value
            // will be -1.
            estimated_heights_px_.emplace_back(value.Int32());
          }
        });
  }
}

// update "estimated-main-axis-size-px" info on radon architecture
void AdapterHelper::UpdateEstimatedSizesPx(
    const pub::Value& estimated_sizes_px) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_ESTIMATED_SIZE);
  estimated_sizes_px_.clear();
  if (estimated_sizes_px.IsArray()) {
    estimated_sizes_px.ForeachArray(
        [this](int64_t index, const pub::Value& value) {
          if (value.IsInt32()) {
            // Note: In radon arch, if not set estimated_sizes_px, the value
            // will be -1.
            estimated_sizes_px_.emplace_back(value.Int32());
          }
        });
  }
}

// update "full-span" info on radon architecture
void AdapterHelper::UpdateFullSpans(const pub::Value& full_spans) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_FULL_SPANS);
  full_spans_.clear();
  if (full_spans.IsArray()) {
    full_spans.ForeachArray([this](int64_t index, const pub::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        full_spans_.insert(value.Int32());
      }
    });
  }
}

// update "sticky-bottom" info on radon architecture
void AdapterHelper::UpdateStickyBottoms(const pub::Value& sticky_bottoms) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_STICKY_BOTTOMS);
  sticky_bottoms_.clear();
  if (sticky_bottoms.IsArray()) {
    sticky_bottoms.ForeachArray([this](int64_t index, const pub::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        sticky_bottoms_.emplace_back(value.Int32());
      }
    });
  }
}

// update "sticky-top" info on radon architecture
void AdapterHelper::UpdateStickyTops(const pub::Value& sticky_tops) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_STICKY_TOPS);
  sticky_tops_.clear();
  if (sticky_tops.IsArray()) {
    sticky_tops.ForeachArray([this](int64_t index, const pub::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        sticky_tops_.emplace_back(value.Int32());
      }
    });
  }
}

// update "insert-action" on fiber architecture
void AdapterHelper::UpdateFiberInsertAction(
    const std::unique_ptr<pub::Value>& insert_action,
    bool only_parse_insertions /* = true */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_FIBER_INSERT_ACTION);
  if (!insert_action || !insert_action->IsArray()) {
    return;
  }
  auto& insertions = diff_result_.insertions_;
  auto& item_keys = diff_result_.item_keys_;
  if (only_parse_insertions) {
    insertions.clear();
  }
  std::ostringstream oss;
  insert_action->ForeachArray([this, only_parse_insertions, &oss, &insertions,
                               &item_keys](int64_t index,
                                           const pub::Value& value) {
    if (value.IsMap()) {
      const auto& position = value.GetValueForKey(kFiberDataPosition);
      const auto& item_key = value.GetValueForKey(kFiberDataItemKey);
      if (!position || !item_key || !position->IsNumber()) {
        return;
      }
      int index = static_cast<int>(position->Number());
      if (item_key->IsString() && !item_key->str().empty()) {
        const auto& item_key_str = item_key->str();
        if (index >= 0) {
          if (only_parse_insertions) {
            insertions.emplace_back(index);
            return;
          }
          if (index <= static_cast<int>(item_keys.size())) {
            const auto& is_full_span = value.GetValueForKey(kFiberDataFullSpan);
            const auto& is_sticky_top =
                value.GetValueForKey(kFiberDataStickyTop);
            const auto& is_sticky_bottom =
                value.GetValueForKey(kFiberDataStickyBottom);
            const auto& estimated_height_px =
                value.GetValueForKey(kFiberDataEstimatedHeightPx);
            const auto& estimated_size_px =
                value.GetValueForKey(kFiberDataEstimatedMainAxisSizePx);
            const auto& recyclable = value.GetValueForKey(kFiberDataRecyclable);
            item_keys.insert(item_keys.begin() + index, item_key_str);
            if (is_full_span && is_full_span->IsBool() &&
                is_full_span->Bool()) {
              fiber_full_spans_.insert(item_key_str);
            }
            if (is_sticky_top && is_sticky_top->IsBool() &&
                is_sticky_top->Bool()) {
              fiber_sticky_tops_.insert(item_key_str);
            }
            if (is_sticky_bottom && is_sticky_bottom->IsBool() &&
                is_sticky_bottom->Bool()) {
              fiber_sticky_bottoms_.insert(item_key_str);
            }
            if (estimated_height_px && estimated_height_px->IsNumber()) {
              fiber_estimated_heights_px_[item_key_str] =
                  static_cast<int32_t>(estimated_height_px->Number());
            }
            if (estimated_size_px && estimated_size_px->IsNumber()) {
              fiber_estimated_sizes_px_[item_key_str] =
                  static_cast<int32_t>(estimated_size_px->Number());
            }
            if (recyclable && recyclable->IsBool() && !recyclable->Bool()) {
              fiber_unrecyclable_.insert(item_key_str);
            }
          }
        }
      } else if (!only_parse_insertions) {
        // Parse illegal item-key
        if (oss.str().empty()) {
          oss << "indexes: [";
        }
        oss << index << ", ";
      }
    }
  });
  if (!only_parse_insertions && delegate_ && !oss.str().empty()) {
    std::string error_msg =
        "Error for illegal list item-key in parse insert with " + oss.str() +
        "]";
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_ILLEGAL_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
}

// update "remove-action" on  fiber architecture
void AdapterHelper::UpdateFiberRemoveAction(
    const std::unique_ptr<pub::Value>& remove_action,
    bool only_parse_removals /* = true */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_FIBER_REMOVE_ACTION);
  if (!remove_action || !remove_action->IsArray()) {
    return;
  }
  auto& removals = diff_result_.removals_;
  auto& item_keys = diff_result_.item_keys_;
  if (only_parse_removals) {
    removals.clear();
  }
  remove_action->ForeachArray([this, only_parse_removals, &removals,
                               &item_keys](int64_t array_index,
                                           const pub::Value& value) {
    if (!value.IsNumber()) {
      return;
    }
    int index = static_cast<int>(value.Number());
    if (index >= 0 && index < static_cast<int>(item_keys.size())) {
      if (only_parse_removals) {
        removals.emplace_back(index);
        return;
      }
      // Note: item_keys_ is a vector, and can not remove element from it in the
      // forward direction.
      const auto& item_key_str = item_keys[index];
      auto it = item_key_map_.end();
      if (item_key_map_.end() != (it = item_key_map_.find(item_key_str))) {
        item_key_map_.erase(it);
      }
      if (fiber_full_spans_.end() != fiber_full_spans_.find(item_key_str)) {
        fiber_full_spans_.erase(item_key_str);
      }
      if (fiber_sticky_tops_.end() != fiber_sticky_tops_.find(item_key_str)) {
        fiber_sticky_tops_.erase(item_key_str);
      }
      if (fiber_sticky_bottoms_.end() !=
          fiber_sticky_bottoms_.find(item_key_str)) {
        fiber_sticky_bottoms_.erase(item_key_str);
      }
      if (fiber_estimated_heights_px_.end() !=
          fiber_estimated_heights_px_.find(item_key_str)) {
        fiber_estimated_heights_px_.erase(item_key_str);
      }
      if (fiber_estimated_sizes_px_.end() !=
          fiber_estimated_sizes_px_.find(item_key_str)) {
        fiber_estimated_sizes_px_.erase(item_key_str);
      }
      if (fiber_unrecyclable_.end() != fiber_unrecyclable_.find(item_key_str)) {
        fiber_unrecyclable_.erase(item_key_str);
      }
    }
  });
  if (!only_parse_removals) {
    item_keys.clear();
    std::vector<std::pair<std::string, int>> remaining_item_keys(
        item_key_map_.begin(), item_key_map_.end());

    std::sort(remaining_item_keys.begin(), remaining_item_keys.end(),
              [](const std::pair<std::string, int>& l,
                 const std::pair<std::string, int>& r) {
                return l.second < r.second;
              });
    for (const auto& it : remaining_item_keys) {
      item_keys.emplace_back(it.first);
    }
  }
}

// update "update-action" on fiber architecture
void AdapterHelper::UpdateFiberUpdateAction(
    const std::unique_ptr<pub::Value>& update_action,
    bool only_parse_update /* = true */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_FIBER_UPDATE_ACTION);
  if (!update_action || !update_action->IsArray()) {
    return;
  }
  auto& update_from = diff_result_.update_from_;
  auto& update_to = diff_result_.update_to_;
  auto& item_keys = diff_result_.item_keys_;
  if (only_parse_update) {
    update_from.clear();
    update_to.clear();
  }
  std::ostringstream oss;
  update_action->ForeachArray([this, only_parse_update, &update_from,
                               &update_to, &item_keys,
                               &oss](int64_t index, const pub::Value& value) {
    if (value.IsMap()) {
      const auto& from_position = value.GetValueForKey(kFiberDataFrom);
      const auto& to_position = value.GetValueForKey(kFiberDataTo);
      const auto& item_key = value.GetValueForKey(kFiberDataItemKey);
      const auto& flush = value.GetValueForKey(kFiberDataFlush);
      if (!from_position || !to_position || !flush || !item_key ||
          !from_position->IsNumber() || !to_position->IsNumber() ||
          !flush->IsBool()) {
        return;
      }
      int from = static_cast<int>(from_position->Number());
      int to = static_cast<int>(to_position->Number());
      if (item_key->IsString() && !item_key->str().empty()) {
        if (from >= 0 && to >= 0) {
          if (only_parse_update) {
            if (flush->Bool()) {
              update_from.emplace_back(from);
              update_to.emplace_back(to);
            }
            // Note: if only_parse_update == true, we should return here to
            // avoid updating item_keys_ repeatedly.
            return;
          }
          const std::string& item_key_str = item_key->str();
          if (from < static_cast<int>(item_keys.size())) {
            item_keys[from] = item_key_str;
            const auto& is_full_span = value.GetValueForKey(kFiberDataFullSpan);
            const auto& is_sticky_top =
                value.GetValueForKey(kFiberDataStickyTop);
            const auto& is_sticky_bottom =
                value.GetValueForKey(kFiberDataStickyBottom);
            const auto& estimated_height_px =
                value.GetValueForKey(kFiberDataEstimatedHeightPx);
            const auto& estimated_size_px =
                value.GetValueForKey(kFiberDataEstimatedMainAxisSizePx);
            const auto& recyclable = value.GetValueForKey(kFiberDataRecyclable);
            if (is_full_span && is_full_span->IsBool()) {
              if (is_full_span->Bool()) {
                fiber_full_spans_.insert(item_key_str);
              } else {
                fiber_full_spans_.erase(item_key_str);
              }
            }
            if (is_sticky_top && is_sticky_top->IsBool()) {
              if (is_sticky_top->Bool()) {
                fiber_sticky_tops_.insert(item_key_str);
              } else {
                fiber_sticky_tops_.erase(item_key_str);
              }
            }
            if (is_sticky_bottom && is_sticky_bottom->IsBool()) {
              if (is_sticky_bottom->Bool()) {
                fiber_sticky_bottoms_.insert(item_key_str);
              } else {
                fiber_sticky_bottoms_.erase(item_key_str);
              }
            }
            if (estimated_height_px && estimated_height_px->IsNumber() &&
                fiber_estimated_heights_px_.end() !=
                    fiber_estimated_heights_px_.find(item_key_str)) {
              fiber_estimated_heights_px_[item_key_str] =
                  static_cast<int32_t>(estimated_height_px->Number());
            }
            if (estimated_size_px && estimated_size_px->IsNumber() &&
                fiber_estimated_sizes_px_.end() !=
                    fiber_estimated_sizes_px_.find(item_key_str)) {
              fiber_estimated_sizes_px_[item_key_str] =
                  static_cast<int32_t>(estimated_size_px->Number());
            }
            if (recyclable && recyclable->IsBool()) {
              if (!recyclable->Bool()) {
                fiber_unrecyclable_.insert(item_key_str);
              } else {
                fiber_unrecyclable_.erase(item_key_str);
              }
            }
          }
        }
      } else if (!only_parse_update) {
        // Parse illegal item-key
        if (oss.str().empty()) {
          oss << "indexes: [";
        }
        oss << from << ", ";
      }
    }
  });
  if (!only_parse_update && delegate_ && !oss.str().empty()) {
    std::string error_msg =
        "Error for illegal list item-key in parse update with " + oss.str() +
        "]";
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_ILLEGAL_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
}

// update extra info such as sticky、full-span on fiber architecture
void AdapterHelper::UpdateFiberExtraInfo() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADAPTER_HELPER_UPDATE_FIBER_EXTRA_INFO);
  // update item_key_map_ from item_keys_ vector after parse insert / remove /
  // update actions
  auto& item_keys = diff_result_.item_keys_;
  bool has_duplicated_item_key = false;
  item_key_map_.clear();
  for (int i = 0; i < static_cast<int>(item_keys.size()); ++i) {
    const std::string& item_key = item_keys[i];
    has_duplicated_item_key =
        has_duplicated_item_key ||
        item_key_map_.find(item_key) != item_key_map_.end();
    item_key_map_[item_key] = i;
  }

  if (has_duplicated_item_key && delegate_) {
    std::string error_msg = "Error for duplicated list item-key. ";
    error_msg += "Last diff result is " + last_diff_result_.ToString();
    error_msg += "Current diff result is " + diff_result_.ToString();
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_DUPLICATE_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
  // update estimated height px from fiber
  estimated_heights_px_.resize(item_keys.size(), -1);
  for (const auto& pair : fiber_estimated_heights_px_) {
    auto it = item_key_map_.find(pair.first);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys.size())) {
      estimated_heights_px_[it->second] = pair.second;
    }
  }
  // update estimated main axis size px from fiber
  estimated_sizes_px_.resize(item_keys.size(), -1);
  for (const auto& pair : fiber_estimated_sizes_px_) {
    auto it = item_key_map_.find(pair.first);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys.size())) {
      estimated_sizes_px_[it->second] = pair.second;
    }
  }
  // update full span from fiber
  full_spans_.clear();
  for (const auto& item_key_str : fiber_full_spans_) {
    auto it = item_key_map_.find(item_key_str);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys.size())) {
      full_spans_.insert(it->second);
    }
  }
  // update sticky top from fiber
  sticky_tops_.clear();
  for (const auto& item_key_str : fiber_sticky_tops_) {
    auto it = item_key_map_.find(item_key_str);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys.size())) {
      sticky_tops_.emplace_back(it->second);
    }
  }
  std::sort(sticky_tops_.begin(), sticky_tops_.end());
  // update sticky bottom from fiber
  sticky_bottoms_.clear();
  for (const auto& item_key_str : fiber_sticky_bottoms_) {
    auto it = item_key_map_.find(item_key_str);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys.size())) {
      sticky_bottoms_.emplace_back(it->second);
    }
  }
  std::sort(sticky_bottoms_.begin(), sticky_bottoms_.end());
  // update recyclable from fiber
  unrecyclable_.clear();
  for (const auto& item_key_str : fiber_unrecyclable_) {
    auto it = item_key_map_.find(item_key_str);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys.size())) {
      unrecyclable_.insert(it->second);
    }
  }
  // save last diff result.
  last_diff_result_ = diff_result_;
}

}  // namespace list
}  // namespace lynx
