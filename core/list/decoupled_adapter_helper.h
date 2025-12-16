// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_ADAPTER_HELPER_H_
#define CORE_LIST_DECOUPLED_ADAPTER_HELPER_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "core/list/decoupled_list_types.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace list {

class AdapterHelper {
 public:
  class Delegate {
   public:
    virtual void OnErrorOccurred(lynx::base::LynxError error) = 0;
  };

  class DiffResult {
   public:
    std::vector<std::string> item_keys_;
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
  // Update diff info for Radon arch.
  bool UpdateRadonDiffResult(const pub::Value& diff_result);

  void UpdateItemKeys(const pub::Value& item_keys_value);

  void UpdateEstimatedHeightsPx(const pub::Value& estimated_heights_px);

  void UpdateEstimatedSizesPx(const pub::Value& estimated_sizes_px);

  void UpdateFullSpans(const pub::Value& full_spans);

  void UpdateStickyBottoms(const pub::Value& sticky_bottoms);

  void UpdateStickyTops(const pub::Value& sticky_tops);

  // Update diff info for Fiber arch.
  void UpdateFiberInsertAction(const std::unique_ptr<pub::Value>& insert_action,
                               bool only_parse_insertions = true);

  void UpdateFiberRemoveAction(const std::unique_ptr<pub::Value>& remove_action,
                               bool only_parse_removals = true);

  void UpdateFiberUpdateAction(const std::unique_ptr<pub::Value>& update_action,
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
    auto it = item_key_map_.end();
    if (item_key_map_.end() != (it = item_key_map_.find(item_key))) {
      return it->second;
    }
    return kInvalidIndex;
  }

  bool HasExpectedDiffAnimation() const {
    return !(insertions().size() == item_keys().size() &&
             update_from().empty() && update_to().empty() &&
             move_from().empty() && move_to().empty() && removals().empty());
  }

  void ClearDiffResult() {
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
    return item_key_map_;
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
  inline const std::unordered_set<int32_t>& unrecyclable() const {
    return unrecyclable_;
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

 private:
  void UpdateInsertions(const pub::Value& diff_insertions);

  void UpdateRemovals(const pub::Value& diff_removals);

  void UpdateUpdateFrom(const pub::Value& diff_update_from);

  void UpdateUpdateTo(const pub::Value& diff_update_to);

  void UpdateMoveTo(const pub::Value& diff_move_to);

  void UpdateMoveFrom(const pub::Value& diff_move_from);

 private:
  AdapterHelper::Delegate* delegate_{nullptr};
  DiffResult diff_result_;
  DiffResult last_diff_result_;
  // the data structure of the map is the <item-key,position>
  std::unordered_map<std::string, int> item_key_map_;
  std::unordered_set<int32_t> full_spans_;
  std::vector<int32_t> sticky_tops_;
  std::vector<int32_t> sticky_bottoms_;
  // Deprecated: using estimated_sizes_px_
  std::vector<int32_t> estimated_heights_px_;
  std::vector<int32_t> estimated_sizes_px_;
  std::unordered_set<int32_t> unrecyclable_;
  // Fiber
  // Deprecated: using fiber_estimated_sizes_px_
  std::unordered_map<std::string, int32_t> fiber_estimated_heights_px_;
  std::unordered_map<std::string, int32_t> fiber_estimated_sizes_px_;
  std::unordered_set<std::string> fiber_full_spans_;
  std::unordered_set<std::string> fiber_sticky_tops_;
  std::unordered_set<std::string> fiber_sticky_bottoms_;
  std::unordered_set<std::string> fiber_unrecyclable_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_ADAPTER_HELPER_H_
