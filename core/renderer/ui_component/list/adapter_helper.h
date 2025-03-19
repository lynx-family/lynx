// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_ADAPTER_HELPER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_ADAPTER_HELPER_H_

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class AdapterHelper {
 private:
  void UpdateInsertions(const lepus::Value& diff_insertions);
  void UpdateRemovals(const lepus::Value& diff_removals);
  void UpdateUpdateFrom(const lepus::Value& diff_update_from);
  void UpdateUpdateTo(const lepus::Value& diff_update_to);
  void UpdateMoveTo(const lepus::Value& diff_move_to);
  void UpdateMoveFrom(const lepus::Value& diff_move_from);

 public:
  class Delegate {
   public:
    virtual void OnErrorOccurred(lynx::base::LynxError error) = 0;
  };

  class DiffResult {
   public:
    std::vector<std::string> item_keys_;
    // the data structure of the map is the <item-key,position>
    std::unordered_map<std::string, int> item_key_map_;
    std::vector<int32_t> insertions_;
    std::vector<int32_t> removals_;
    std::vector<int32_t> update_from_;
    std::vector<int32_t> update_to_;
    std::vector<int32_t> move_from_;
    std::vector<int32_t> move_to_;

    bool HasValidDiff() const {
      return insertions_.size() > 0 || removals_.size() > 0 ||
             move_to_.size() > 0 || move_from_.size() > 0 ||
             update_to_.size() > 0 || update_from_.size() > 0;
    }
    std::string ToString() const;
  };
  // radon-diff
  bool UpdateDiffResult(const lepus::Value& diff_result);
  void UpdateItemKeys(const lepus::Value& item_keys_value);
  void UpdateEstimatedHeightsPx(const lepus::Value& estimated_heights_px);
  void UpdateEstimatedSizesPx(const lepus::Value& estimated_sizes_px);
  void UpdateFullSpans(const lepus::Value& full_spans);
  void UpdateStickyBottoms(const lepus::Value& sticky_bottoms);
  void UpdateStickyTops(const lepus::Value& sticky_tops);
  // Fiber
  void UpdateFiberInsertAction(const lepus::Value& insert_action,
                               bool only_parse_insertions = true);
  void UpdateFiberRemoveAction(const lepus::Value& remove_action,
                               bool only_parse_removals = true);
  void UpdateFiberUpdateAction(const lepus::Value& update_action,
                               bool only_parse_update = true);
  void UpdateFiberExtraInfo();

  void SetDelegate(AdapterHelper::Delegate* delegate) { delegate_ = delegate; }

  int32_t GetDateCount() const {
    return static_cast<int32_t>(diff_result_.item_keys_.size());
  }

  std::optional<std::string> GetItemKeyForIndex(int index) const {
    if (index >= 0 && index < GetDateCount()) {
      return std::make_optional<std::string>(diff_result_.item_keys_[index]);
    }
    return std::nullopt;
  }

  bool HasValidDiff() const { return diff_result_.HasValidDiff(); }

  int GetIndexForItemKey(const std::string& item_key) const {
    const auto& item_key_map = diff_result_.item_key_map_;
    auto it = item_key_map.end();
    if (item_key_map.end() != (it = item_key_map.find(item_key))) {
      return it->second;
    }
    return list::kInvalidIndex;
  }

  void ClearDiffInfo() {
    diff_result_.insertions_.clear();
    diff_result_.removals_.clear();
    diff_result_.update_from_.clear();
    diff_result_.update_to_.clear();
    diff_result_.move_from_.clear();
    diff_result_.move_to_.clear();
  }

  inline const std::vector<std::string>& item_keys() const {
    return diff_result_.item_keys_;
  }
  inline const std::vector<int32_t>& estimated_heights_px() const {
    return estimated_heights_px_;
  }
  inline const std::vector<int32_t>& estimated_sizes_px() const {
    return estimated_sizes_px_;
  }
  inline const std::unordered_map<std::string, int>& item_key_map() const {
    return diff_result_.item_key_map_;
  }
  inline const std::unordered_set<int32_t>& full_spans() const {
    return full_spans_;
  }
  inline const std::vector<int32_t>& sticky_bottoms() const {
    return sticky_bottoms_;
  }
  inline const std::vector<int32_t>& sticky_tops() const {
    return sticky_tops_;
  }
  inline const std::vector<int32_t>& insertions() const {
    return diff_result_.insertions_;
  }
  inline const std::vector<int32_t>& removals() const {
    return diff_result_.removals_;
  }
  inline const std::vector<int32_t>& update_from() const {
    return diff_result_.update_from_;
  }
  inline const std::vector<int32_t>& update_to() const {
    return diff_result_.update_to_;
  }
  inline const std::vector<int32_t>& move_from() const {
    return diff_result_.move_from_;
  }
  inline const std::vector<int32_t>& move_to() const {
    return diff_result_.move_to_;
  }
  fml::RefPtr<lepus::Dictionary> GenerateDiffInfo() const;

 private:
  AdapterHelper::Delegate* delegate_{nullptr};
  DiffResult diff_result_;
  DiffResult last_diff_result_;
  std::unordered_set<int32_t> full_spans_;
  std::vector<int32_t> sticky_tops_;
  std::vector<int32_t> sticky_bottoms_;
  // Deprecated: using estimated_sizes_px_
  std::vector<int32_t> estimated_heights_px_;
  std::vector<int32_t> estimated_sizes_px_;
  // Fiber
  // Deprecated: using fiber_estimated_sizes_px_
  std::unordered_map<std::string, int32_t> fiber_estimated_heights_px_;
  std::unordered_map<std::string, int32_t> fiber_estimated_sizes_px_;
  std::unordered_set<std::string> fiber_full_spans_;
  std::unordered_set<std::string> fiber_sticky_tops_;
  std::unordered_set<std::string> fiber_sticky_bottoms_;
};
}  // namespace tasm
}  // namespace lynx

#endif  //  CORE_RENDERER_UI_COMPONENT_LIST_ADAPTER_HELPER_H_
