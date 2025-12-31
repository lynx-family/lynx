// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_RADON_DATA_SOURCE_H_
#define CORE_LIST_TESTING_RADON_DATA_SOURCE_H_

#include <string>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {
namespace testing {

class RadonDataSource {
 public:
  std::vector<std::string> item_keys_;
  std::vector<int> insertion_;
  std::vector<int> removal_;
  std::vector<int> update_from_;
  std::vector<int> update_to_;
  std::vector<int> estimated_height_pxs_;
  std::vector<int> estimated_main_axis_size_pxs_;
  std::vector<int> sticky_tops_;
  std::vector<int> sticky_bottoms_;
  std::vector<int> full_spans_;

  int GetItemCount() const { return static_cast<int>(item_keys_.size()); }

  fml::RefPtr<lepus::Dictionary> GenerateDataSource() const {
    // item keys
    auto item_keys_array = lepus::CArray::Create();
    for (const auto& item_key : item_keys_) {
      item_keys_array->push_back(lepus_value(item_key));
    }
    // estimated_height_px
    auto estimated_height_px_array = lepus::CArray::Create();
    for (const auto& height : estimated_height_pxs_) {
      estimated_height_px_array->push_back(lepus_value(height));
    }
    // estimated_main_axis_size_px
    auto estimated_main_axis_px_array = lepus::CArray::Create();
    for (const auto& height : estimated_main_axis_size_pxs_) {
      estimated_main_axis_px_array->push_back(lepus_value(height));
    }
    // sticky_tops
    auto sticky_tops_array = lepus::CArray::Create();
    for (int32_t index : sticky_tops_) {
      sticky_tops_array->push_back(lepus_value(index));
    }
    // sticky_bottoms
    auto sticky_bottoms_array = lepus::CArray::Create();
    for (int32_t index : sticky_bottoms_) {
      sticky_bottoms_array->push_back(lepus_value(index));
    }
    // full_spans
    auto full_spans_array = lepus::CArray::Create();
    for (int32_t index : full_spans_) {
      full_spans_array->push_back(lepus_value(index));
    }
    // insertions
    auto insertion_array = lepus::CArray::Create();
    for (const auto& index : insertion_) {
      insertion_array->push_back(lepus_value(index));
    }
    // removals
    auto removal_array = lepus::CArray::Create();
    for (const auto& index : removal_) {
      removal_array->push_back(lepus_value(index));
    }
    // update_from
    auto update_from_array = lepus::CArray::Create();
    for (const auto& index : update_from_) {
      update_from_array->push_back(lepus_value(index));
    }
    // update_to
    auto update_to_array = lepus::CArray::Create();
    for (const auto& index : update_to_) {
      update_to_array->push_back(lepus_value(index));
    }
    auto diff_result = lepus::Dictionary::Create();
    diff_result->SetValue(kRadonDataInsertions, lepus_value(insertion_array));
    diff_result->SetValue(kRadonDataRemovals, lepus_value(removal_array));
    diff_result->SetValue(kRadonDataUpdateFrom, lepus_value(update_from_array));
    diff_result->SetValue(kRadonDataUpdateTo, lepus_value(update_to_array));
    auto data_source = lepus::Dictionary::Create();
    data_source->SetValue(kRadonDataDiffResult, lepus_value(diff_result));
    data_source->SetValue(kRadonDataItemKeys, lepus_value(item_keys_array));
    data_source->SetValue(kRadonDataEstimatedHeightPx,
                          lepus_value(estimated_height_px_array));
    data_source->SetValue(kRadonDataEstimatedMainAxisSizePx,
                          lepus_value(estimated_main_axis_px_array));
    data_source->SetValue(kRadonDataFullSpan, lepus_value(full_spans_array));
    data_source->SetValue(kRadonDataStickyTop, lepus_value(sticky_tops_array));
    data_source->SetValue(kRadonDataStickyBottom,
                          lepus_value(sticky_bottoms_array));
    return data_source;
  }
};

}  // namespace testing
}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_RADON_DATA_SOURCE_H_
