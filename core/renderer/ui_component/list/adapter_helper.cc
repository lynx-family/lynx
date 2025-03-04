// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/adapter_helper.h"

#include <algorithm>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {
//  update "diff-result" info  on radon_diff architecture
bool AdapterHelper::UpdateDiffResult(const lepus::Value& diff_result) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateDiffResult");
  bool has_update = false;
  if (diff_result.IsObject()) {
    ForEachLepusValue(diff_result,
                      [this, &has_update](const lepus::Value& key,
                                          const lepus::Value& value) {
                        const auto& key_str = key.StdString();
                        if (key_str == list::kInsertions) {
                          UpdateInsertions(value);
                          has_update = true;
                        } else if (key_str == list::kRemovals) {
                          UpdateRemovals(value);
                          has_update = true;
                        } else if (key_str == list::kUpdateFrom) {
                          UpdateUpdateFrom(value);
                          has_update = true;
                        } else if (key_str == list::kUpdateTo) {
                          UpdateUpdateTo(value);
                          has_update = true;
                        } else if (key_str == list::kMoveFrom) {
                          UpdateMoveFrom(value);
                          has_update = true;
                        } else if (key_str == list::kMoveTo) {
                          UpdateMoveTo(value);
                          has_update = true;
                        }
                      });
  }
  return has_update;
}

void AdapterHelper::UpdateInsertions(const lepus::Value& diff_insertions) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateInsertions");
  insertions_.clear();
  if (diff_insertions.IsArray()) {
    ForEachLepusValue(diff_insertions, [this](const lepus::Value& key,
                                              const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        insertions_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateRemovals(const lepus::Value& diff_removals) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateRemovals");
  removals_.clear();
  if (diff_removals.IsArray()) {
    ForEachLepusValue(diff_removals, [this](const lepus::Value& key,
                                            const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        removals_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateUpdateFrom(const lepus::Value& diff_update_from) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateUpdateFrom");
  update_from_.clear();
  if (diff_update_from.IsArray()) {
    ForEachLepusValue(diff_update_from, [this](const lepus::Value& key,
                                               const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        update_from_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateUpdateTo(const lepus::Value& diff_update_to) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateUpdateTo");
  update_to_.clear();
  if (diff_update_to.IsArray()) {
    ForEachLepusValue(diff_update_to, [this](const lepus::Value& key,
                                             const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        update_to_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateMoveTo(const lepus::Value& diff_move_to) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateMoveTo");
  move_to_.clear();
  if (diff_move_to.IsArray()) {
    ForEachLepusValue(diff_move_to, [this](const lepus::Value& key,
                                           const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        move_to_.emplace_back(value.Int32());
      }
    });
  }
}

void AdapterHelper::UpdateMoveFrom(const lepus::Value& diff_move_from) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateMoveFrom");
  move_from_.clear();
  if (diff_move_from.IsArray()) {
    ForEachLepusValue(diff_move_from, [this](const lepus::Value& key,
                                             const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        move_from_.emplace_back(value.Int32());
      }
    });
  }
}

fml::RefPtr<lepus::Dictionary> AdapterHelper::GenerateDiffInfo() const {
  auto diff_info = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kInsertionsKey, "insertions");
  BASE_STATIC_STRING_DECL(kRemovalsKey, "removals");
  BASE_STATIC_STRING_DECL(kUpdateFromKey, "update_from");
  BASE_STATIC_STRING_DECL(kUpdateToKey, "update_to");
  BASE_STATIC_STRING_DECL(kMoveToKey, "move_to");
  BASE_STATIC_STRING_DECL(kMoveFromKey, "move_from");
  auto array = lepus::CArray::Create();
  for (auto i : insertions_) {
    array->emplace_back(i);
  }
  diff_info->SetValue(kInsertionsKey, std::move(array));
  array = lepus::CArray::Create();
  for (auto i : removals_) {
    array->emplace_back(i);
  }
  diff_info->SetValue(kRemovalsKey, std::move(array));
  array = lepus::CArray::Create();
  for (auto i : update_from_) {
    array->emplace_back(i);
  }
  diff_info->SetValue(kUpdateFromKey, std::move(array));
  array = lepus::CArray::Create();
  for (auto i : update_to_) {
    array->emplace_back(i);
  }
  diff_info->SetValue(kUpdateToKey, std::move(array));
  array = lepus::CArray::Create();
  for (auto i : move_to_) {
    array->emplace_back(i);
  }
  diff_info->SetValue(kMoveToKey, std::move(array));
  array = lepus::CArray::Create();
  for (auto i : move_from_) {
    array->emplace_back(i);
  }
  diff_info->SetValue(kMoveFromKey, std::move(array));
  return diff_info;
}

//   update "item-key" info  on radon_diff architecture
void AdapterHelper::UpdateItemKeys(const lepus::Value& item_keys) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateItemKeys");
  item_keys_.clear();
  item_key_map_.clear();
  bool has_illegal_item_key = false;
  bool has_duplicated_item_key = false;
  if (item_keys.IsArray()) {
    ForEachLepusValue(
        item_keys, [this, &has_illegal_item_key, &has_duplicated_item_key](
                       const lepus::Value& key, const lepus::Value& value) {
          if (value.IsString()) {
            const std::string& item_key = value.StdString();
            has_duplicated_item_key =
                has_duplicated_item_key ||
                item_key_map_.find(item_key) != item_key_map_.end();
            item_key_map_[item_key] = static_cast<int>(item_keys_.size());
            item_keys_.emplace_back(item_key);
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

// update "estimated-height-px" info  on radon_diff architecture
void AdapterHelper::UpdateEstimatedHeightsPx(
    const lepus::Value& estimated_heights_px) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateEstimatedHeightsPx");
  estimated_heights_px_.clear();
  if (estimated_heights_px.IsArray()) {
    ForEachLepusValue(estimated_heights_px, [this](const lepus::Value& key,
                                                   const lepus::Value& value) {
      if (value.IsInt32()) {
        // Note: In radon arch, if not set estimated_heights_px, the value will
        // be -1.
        estimated_heights_px_.emplace_back(value.Int32());
      }
    });
  }
}

// update "estimated-main-axis-size-px" info  on radon_diff architecture
void AdapterHelper::UpdateEstimatedSizesPx(
    const lepus::Value& estimated_sizes_px) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateEstimatedSizesPx");
  estimated_sizes_px_.clear();
  if (estimated_sizes_px.IsArray()) {
    ForEachLepusValue(estimated_sizes_px, [this](const lepus::Value& key,
                                                 const lepus::Value& value) {
      if (value.IsInt32()) {
        // Note: In radon arch, if not set estimated_sizes_px, the value will
        // be -1.
        estimated_sizes_px_.emplace_back(value.Int32());
      }
    });
  }
}

//  update "full-span" info  on radon_diff architecture
void AdapterHelper::UpdateFullSpans(const lepus::Value& full_spans) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateFullSpans");
  full_spans_.clear();
  if (full_spans.IsArray()) {
    ForEachLepusValue(
        full_spans, [this](const lepus::Value& key, const lepus::Value& value) {
          if (value.IsInt32() && value.Int32() >= 0) {
            full_spans_.insert(value.Int32());
          }
        });
  }
}

// update "sticky-bottom" info  on radon_diff architecture
void AdapterHelper::UpdateStickyBottoms(const lepus::Value& sticky_bottoms) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateStickyBottoms");
  sticky_bottoms_.clear();
  if (sticky_bottoms.IsArray()) {
    ForEachLepusValue(sticky_bottoms, [this](const lepus::Value& key,
                                             const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        sticky_bottoms_.emplace_back(value.Int32());
      }
    });
  }
}

// update "sticky-top" info  on radon_diff architecture
void AdapterHelper::UpdateStickyTops(const lepus::Value& sticky_tops) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateStickyTops");
  sticky_tops_.clear();
  if (sticky_tops.IsArray()) {
    ForEachLepusValue(sticky_tops, [this](const lepus::Value& key,
                                          const lepus::Value& value) {
      if (value.IsInt32() && value.Int32() >= 0) {
        sticky_tops_.emplace_back(value.Int32());
      }
    });
  }
}

// update "insert-action" on fiber architecture
void AdapterHelper::UpdateFiberInsertAction(const lepus::Value& insert_action,
                                            bool only_parse_insertions) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateFiberInsertAction");
  if (!insert_action.IsArray()) {
    return;
  }
  if (only_parse_insertions) {
    insertions_.clear();
  }
  bool has_illegal_item_key = false;
  ForEachLepusValue(
      insert_action, [this, only_parse_insertions, &has_illegal_item_key](
                         const lepus::Value& key, const lepus::Value& value) {
        if (!value.IsTable()) {
          return;
        }
        const auto& position =
            value.GetProperty(BASE_STATIC_STRING(list::kPosition));
        const auto& item_key =
            value.GetProperty(BASE_STATIC_STRING(list::kItemKey));
        if (position.IsNumber() && item_key.IsString()) {
          int index = static_cast<int>(position.Number());
          const std::string& item_key_str = item_key.StdString();
          if (index >= 0 && !item_key_str.empty()) {
            if (only_parse_insertions) {
              insertions_.emplace_back(index);
              return;
            }
            if (index <= static_cast<int>(item_keys_.size())) {
              const auto& is_full_span =
                  value.GetProperty(BASE_STATIC_STRING(list::kFullSpan));
              const auto& is_sticky_top =
                  value.GetProperty(BASE_STATIC_STRING(list::kStickyTop));
              const auto& is_sticky_bottom =
                  value.GetProperty(BASE_STATIC_STRING(list::kStickyBottom));
              const auto& estimated_height_px = value.GetProperty(
                  BASE_STATIC_STRING(list::kEstimatedHeightPx));
              const auto& estimated_size_px = value.GetProperty(
                  BASE_STATIC_STRING(list::kEstimatedMainAxisSizePx));
              item_keys_.insert(item_keys_.begin() + index, item_key_str);
              if (is_full_span.IsBool() && is_full_span.Bool()) {
                fiber_full_spans_.insert(item_key_str);
              }
              if (is_sticky_top.IsBool() && is_sticky_top.Bool()) {
                fiber_sticky_tops_.insert(item_key_str);
              }
              if (is_sticky_bottom.IsBool() && is_sticky_bottom.Bool()) {
                fiber_sticky_bottoms_.insert(item_key_str);
              }
              if (estimated_height_px.IsNumber()) {
                fiber_estimated_heights_px_[item_key_str] =
                    static_cast<int32_t>(estimated_height_px.Number());
              }
              if (estimated_size_px.IsNumber()) {
                fiber_estimated_sizes_px_[item_key_str] =
                    static_cast<int32_t>(estimated_size_px.Number());
              }
            }
          }
        } else if (!item_key.IsString()) {
          has_illegal_item_key = true;
        }
      });
  if (has_illegal_item_key && !only_parse_insertions && delegate_) {
    std::string error_msg = "Error for illegal list item-key.";
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_ILLEGAL_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
}

// update "remove-action" on  fiber architecture
void AdapterHelper::UpdateFiberRemoveAction(const lepus::Value& remove_action,
                                            bool only_parse_removals) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateFiberRemoveAction");
  if (!remove_action.IsArray()) {
    return;
  }
  if (only_parse_removals) {
    removals_.clear();
  }
  ForEachLepusValue(remove_action, [this, only_parse_removals](
                                       const lepus::Value& key,
                                       const lepus::Value& value) {
    if (!value.IsNumber()) {
      return;
    }
    int index = static_cast<int>(value.Number());
    if (index >= 0 && index < static_cast<int>(item_keys_.size())) {
      if (only_parse_removals) {
        removals_.emplace_back(index);
        return;
      }
      // Note: item_keys_ is a vector, and can not remove element from it in the
      // forward direction.
      const auto& item_key_str = item_keys_[index];
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
    }
  });
  if (!only_parse_removals) {
    item_keys_.clear();
    std::vector<std::pair<std::string, int>> remaining_item_keys(
        item_key_map_.begin(), item_key_map_.end());

    std::sort(remaining_item_keys.begin(), remaining_item_keys.end(),
              [](const std::pair<std::string, int>& l,
                 const std::pair<std::string, int>& r) {
                return l.second < r.second;
              });
    for (const auto& it : remaining_item_keys) {
      item_keys_.emplace_back(it.first);
    }
  }
}

// update "update-action" on fiber architecture
void AdapterHelper::UpdateFiberUpdateAction(const lepus::Value& update_action,
                                            bool only_parse_update) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateFiberUpdateAction");
  if (!update_action.IsArray()) {
    return;
  }
  if (only_parse_update) {
    update_from_.clear();
    update_to_.clear();
  }
  bool has_illegal_item_key = false;
  ForEachLepusValue(
      update_action, [this, only_parse_update, &has_illegal_item_key](
                         const lepus::Value& key, const lepus::Value& value) {
        if (!value.IsTable()) {
          return;
        }
        const auto& from_position =
            value.GetProperty(BASE_STATIC_STRING(list::kFrom));
        const auto& to_position =
            value.GetProperty(BASE_STATIC_STRING(list::kTo));
        const auto& item_key =
            value.GetProperty(BASE_STATIC_STRING(list::kItemKey));
        const auto& flush = value.GetProperty(BASE_STATIC_STRING(list::kFlush));
        if (from_position.IsNumber() && to_position.IsNumber() &&
            item_key.IsString() && flush.IsBool()) {
          int from = static_cast<int>(from_position.Number());
          int to = static_cast<int>(to_position.Number());
          if (from >= 0 && to >= 0) {
            if (only_parse_update) {
              if (flush.Bool()) {
                update_from_.emplace_back(from);
                update_to_.emplace_back(to);
              }
              // Note: if only_parse_update == true, we should return here to
              // avoid updating item_keys_ repeatedly.
              return;
            }
            const std::string& item_key_str = item_key.StdString();
            if (from < static_cast<int>(item_keys_.size()) &&
                !item_key_str.empty()) {
              item_keys_[from] = item_key_str;
              const auto& is_full_span =
                  value.GetProperty(BASE_STATIC_STRING(list::kFullSpan));
              const auto& is_sticky_top =
                  value.GetProperty(BASE_STATIC_STRING(list::kStickyTop));
              const auto& is_sticky_bottom =
                  value.GetProperty(BASE_STATIC_STRING(list::kStickyBottom));
              const auto& estimated_height_px = value.GetProperty(
                  BASE_STATIC_STRING(list::kEstimatedHeightPx));
              const auto& estimated_size_px = value.GetProperty(
                  BASE_STATIC_STRING(list::kEstimatedMainAxisSizePx));
              if (is_full_span.IsBool()) {
                if (is_full_span.Bool()) {
                  fiber_full_spans_.insert(item_key_str);
                } else {
                  fiber_full_spans_.erase(item_key_str);
                }
              }
              if (is_sticky_top.IsBool()) {
                if (is_sticky_top.Bool()) {
                  fiber_sticky_tops_.insert(item_key_str);
                } else {
                  fiber_sticky_tops_.erase(item_key_str);
                }
              }
              if (is_sticky_bottom.IsBool()) {
                if (is_sticky_bottom.Bool()) {
                  fiber_sticky_bottoms_.insert(item_key_str);
                } else {
                  fiber_sticky_bottoms_.erase(item_key_str);
                }
              }
              if (estimated_height_px.IsNumber() &&
                  fiber_estimated_heights_px_.end() !=
                      fiber_estimated_heights_px_.find(item_key_str)) {
                fiber_estimated_heights_px_[item_key_str] =
                    static_cast<int32_t>(estimated_height_px.Number());
              }
              if (estimated_size_px.IsNumber() &&
                  fiber_estimated_sizes_px_.end() !=
                      fiber_estimated_sizes_px_.find(item_key_str)) {
                fiber_estimated_sizes_px_[item_key_str] =
                    static_cast<int32_t>(estimated_size_px.Number());
              }
            }
          }
        } else if (!item_key.IsString()) {
          has_illegal_item_key = true;
        }
      });
  if (has_illegal_item_key && !only_parse_update && delegate_) {
    std::string error_msg = "Error for illegal list item-key.";
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_ILLEGAL_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
}

// update extra info such as sticky、full-span on fiber architecture
void AdapterHelper::UpdateFiberExtraInfo() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AdapterHelper::UpdateFiberExtraInfo");
  // update item_key_map_ from item_keys_ vector after parse insert / remove /
  // update actions
  bool has_duplicated_item_key = false;
  item_key_map_.clear();
  for (int i = 0; i < static_cast<int>(item_keys_.size()); ++i) {
    const std::string& item_key = item_keys_[i];
    has_duplicated_item_key =
        has_duplicated_item_key ||
        item_key_map_.find(item_key) != item_key_map_.end();
    item_key_map_[item_key] = i;
  }
  if (has_duplicated_item_key && delegate_) {
    std::string error_msg = "Error for duplicated list item-key.";
    std::string suggestion = "Please check the legality of the item-key.";
    auto error = lynx::base::LynxError(
        error::E_COMPONENT_LIST_DUPLICATE_ITEM_KEY, std::move(error_msg),
        std::move(suggestion), base::LynxErrorLevel::Error);
    delegate_->OnErrorOccurred(std::move(error));
  }
  // update estimated height px from fiber
  estimated_heights_px_.resize(item_keys_.size(), -1);
  for (const auto& pair : fiber_estimated_heights_px_) {
    auto it = item_key_map_.find(pair.first);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys_.size())) {
      estimated_heights_px_[it->second] = pair.second;
    }
  }
  // update estimated main axis size px from fiber
  estimated_sizes_px_.resize(item_keys_.size(), -1);
  for (const auto& pair : fiber_estimated_sizes_px_) {
    auto it = item_key_map_.find(pair.first);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys_.size())) {
      estimated_sizes_px_[it->second] = pair.second;
    }
  }
  // update full span from fiber
  full_spans_.clear();
  for (const auto& item_key_str : fiber_full_spans_) {
    auto it = item_key_map_.find(item_key_str);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys_.size())) {
      full_spans_.insert(it->second);
    }
  }
  // update sticky top from fiber
  sticky_tops_.clear();
  for (const auto& item_key_str : fiber_sticky_tops_) {
    auto it = item_key_map_.find(item_key_str);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys_.size())) {
      sticky_tops_.emplace_back(it->second);
    }
  }
  std::sort(sticky_tops_.begin(), sticky_tops_.end());
  // update sticky bottom from fiber
  sticky_bottoms_.clear();
  for (const auto& item_key_str : fiber_sticky_bottoms_) {
    auto it = item_key_map_.find(item_key_str);
    if (item_key_map_.end() != it && it->second >= 0 &&
        it->second < static_cast<int>(item_keys_.size())) {
      sticky_bottoms_.emplace_back(it->second);
    }
  }
  std::sort(sticky_bottoms_.begin(), sticky_bottoms_.end());
}

bool AdapterHelper::HasValidDiff() {
  return insertions_.size() > 0 || removals_.size() > 0 ||
         move_to_.size() > 0 || move_from_.size() > 0 ||
         update_to_.size() > 0 || update_from_.size() > 0;
}

}  // namespace tasm
}  // namespace lynx
