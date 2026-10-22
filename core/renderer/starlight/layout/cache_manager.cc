// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/cache_manager.h"

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "core/renderer/starlight/layout/layout_object.h"

namespace {

using lynx::base::FloatsEqual;
using lynx::base::FloatsLargerOrEqual;
using lynx::starlight::LayoutObject;
using lynx::starlight::OneSideConstraint;

inline bool CheckCache(const OneSideConstraint& current_constraint,
                       const OneSideConstraint& last_constraint,
                       float last_computed_size, bool is_horizontal,
                       LayoutObject* target) {
  /*    (old_mode)      (new_mode)      (fix?)
   * 1  indefinite      indefinite       yes
   * 2  indefinite       definite        yes if old_computed_size == new_size
   * 3  indefinite        at most        yes if old_computed_size <= new_size
   * 4   definite       indefinite       yes if old_computed_size == new_size
   * 5   definite        definite        yes if old_computed_size == new_size
   * 6   definite         at most        yes if old_computed_size == new_size
   * 7   at most        indefinite       no
   * 8   at most         definite        yes if old_computed_size == new_size
   * 9   at most          at most        yes if old_size >= new_size and
   * old_computed_size <= new_size
   *
   * Note that the given space is not necessarily large enough to fit the
   * content as if the constraint is indefinite, when measure mode is at-most
   * and computed size is smaller than given size.
   *
   * For example, given two words in a text view.
   * The given at-most width is equal to the width of 1.5 words.
   * The layout result of the text view will have line break between words,
   * and the width of text view should be as wide as one word,
   * which is smaller than given at-most constraint.
   */

  const auto CheckPercentageBase = [&is_horizontal, &target]() -> bool {
    return target->CanReuseLayoutWithSameSizeAsGivenConstraint(is_horizontal);
  };
  if (last_constraint.Mode() == SLMeasureModeIndefinite) {
    if (current_constraint.Mode() == SLMeasureModeIndefinite) {
      return true;
    } else if (current_constraint.Mode() == SLMeasureModeAtMost) {
      return FloatsLargerOrEqual(current_constraint.Size(), last_computed_size);
    } else {
      return FloatsEqual(current_constraint.Size(), last_computed_size) &&
             CheckPercentageBase();
    }
  } else if (last_constraint.Mode() == SLMeasureModeAtMost) {
    if (current_constraint.Mode() == SLMeasureModeIndefinite) {
      return false;
    } else if (current_constraint.Mode() == SLMeasureModeAtMost) {
      if (FloatsEqual(last_constraint.Size(), current_constraint.Size()) ||
          (FloatsLargerOrEqual(last_constraint.Size(),
                               current_constraint.Size()) &&
           FloatsLargerOrEqual(current_constraint.Size(),
                               last_computed_size))) {
        return true;
      }
      return false;
    } else {
      return FloatsEqual(last_computed_size, current_constraint.Size()) &&
             CheckPercentageBase();
    }
  } else {
    if (current_constraint.Mode() == SLMeasureModeIndefinite ||
        current_constraint.Mode() == SLMeasureModeAtMost) {
      return false;
    } else {
      return FloatsEqual(last_constraint.Size(), current_constraint.Size()) ||
             FloatsEqual(last_computed_size, current_constraint.Size());
    }
  }
}

}  // namespace

namespace lynx {
namespace starlight {

void CacheManager::ResetCache() {
  ResetCssCache();
  ResetComposeCache();
}

void CacheManager::ResetCssCache() {
  for (auto& entry : cache_data_) {
    entry.is_valid_ = false;
  }
  current_cache_index_ = cache_data_.size() - 1;
}

void CacheManager::InsertCacheEntry(const Constraints& constraints,
                                    float border_bound_width,
                                    float border_bound_height) {
  ++current_cache_index_;
  if (current_cache_index_ >= LAYOUT_RESULT_CACHE_SIZE) {
    current_cache_index_ = 0;
  }
  for (size_t idx = 0; idx < cache_data_.size(); ++idx) {
    if (cache_data_[idx].is_valid_ &&
        cache_data_[idx].resolved_constraints_ == constraints) {
      current_cache_index_ = idx;
      break;
    }
  }
  CacheEntry* target_entry;
  if (current_cache_index_ >= cache_data_.size()) {
    target_entry = &cache_data_.emplace_back();
  } else {
    target_entry = &cache_data_[current_cache_index_];
  }
  target_entry->resolved_constraints_ = constraints;
  target_entry->border_bound_width_ = border_bound_width;
  target_entry->border_bound_height_ = border_bound_height;
  target_entry->is_valid_ = true;
}

FindCacheResult CacheManager::FindAvailableCacheEntry(
    const Constraints& constraints, LayoutObject& target) const {
  FindCacheResult result;
  const auto CheckCacheHit = [&constraints,
                              &target](const CacheEntry& entry) -> bool {
    return (entry.is_valid_ &&
            CheckCache(constraints[kHorizontal],
                       entry.resolved_constraints_[kHorizontal],
                       entry.border_bound_width_, true, &target) &&
            CheckCache(constraints[kVertical],
                       entry.resolved_constraints_[kVertical],
                       entry.border_bound_height_, false, &target));
  };
  if (current_cache_index_ < cache_data_.size() &&
      CheckCacheHit(cache_data_[current_cache_index_])) {
    result.cache_ = &cache_data_[current_cache_index_];
    result.is_cache_in_sync_with_current_state = true;
    return result;
  }
  for (size_t idx = 0; idx < cache_data_.size(); ++idx) {
    const auto& entry = cache_data_[idx];
    if (idx != current_cache_index_ && CheckCacheHit(entry)) {
      result.cache_ = &entry;
      result.is_cache_in_sync_with_current_state = false;
      break;
    }
  }
  return result;
}

bool CacheManager::CanReuseComposeMeasure(const ComposeConstraints& constraints,
                                          bool final_measure,
                                          const LayoutObject& target) const {
  if (!compose_measure_cache_) return false;
  const auto& cache = *compose_measure_cache_;
  return cache.valid && cache.constraints == constraints &&
         cache.inset_width == target.GetPaddingAndBorderHorizontal() &&
         cache.inset_height == target.GetPaddingAndBorderVertical() &&
         cache.final_measure == final_measure &&
         target.IsInflowSubTreeInSyncWithLastMeasurement() &&
         !target.GetShouldDisplayNone() &&
         cache.size.width_ == target.GetBorderBoundWidth() &&
         cache.size.height_ == target.GetBorderBoundHeight() &&
         cache.size.baseline_ == target.GetBaseline();
}

void CacheManager::BeginComposeMeasure(const ComposeConstraints& constraints,
                                       bool final_measure,
                                       const LayoutObject& target) {
  ResetCssCache();
  ResetComposeCache();
  if (!compose_measure_cache_) {
    compose_measure_cache_ = std::make_unique<ComposeMeasureCacheEntry>();
  }
  auto& cache = *compose_measure_cache_;
  cache.constraints = constraints;
  cache.inset_width = target.GetPaddingAndBorderHorizontal();
  cache.inset_height = target.GetPaddingAndBorderVertical();
  cache.final_measure = final_measure;
}

void CacheManager::InsertComposeCacheEntry(const LayoutObject& target) {
  auto& cache = *compose_measure_cache_;
  cache.size = FloatSize(target.GetBorderBoundWidth(),
                         target.GetBorderBoundHeight(), target.GetBaseline());
  cache.valid = true;
}

void CacheManager::ResetComposeCache() {
  if (compose_measure_cache_) {
    compose_measure_cache_->valid = false;
  }
}

}  // namespace starlight
}  // namespace lynx
