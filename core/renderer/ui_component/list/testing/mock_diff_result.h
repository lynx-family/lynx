// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_TESTING_MOCK_DIFF_RESULT_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_TESTING_MOCK_DIFF_RESULT_H_

#include <string>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {
namespace list {

class DiffResult {
 public:
  std::vector<std::string> item_keys;
  std::vector<int> insertion;
  std::vector<int> removal;
  std::vector<int> update_from;
  std::vector<int> update_to;
  std::vector<int> estimated_height_pxs;
  std::vector<int> estimated_main_axis_size_pxs;
  std::vector<int> sticky_tops;
  std::vector<int> sticky_bottoms;
  std::vector<int> full_spans;

  int GetItemCount() const { return static_cast<int>(item_keys.size()); }

  fml::RefPtr<lepus::Dictionary> GenerateDiffResult() const {
    // item keys
    auto item_keys_array = lepus::CArray::Create();
    for (const auto& item_key : item_keys) {
      item_keys_array->push_back(lepus_value(item_key));
    }
    // estimated_height_px
    auto estimated_height_px_array = lepus::CArray::Create();
    for (const auto& height : estimated_height_pxs) {
      estimated_height_px_array->push_back(lepus_value(height));
    }
    // estimated_main_axis_size_px
    auto estimated_main_axis_px_array = lepus::CArray::Create();
    for (const auto& height : estimated_main_axis_size_pxs) {
      estimated_main_axis_px_array->push_back(lepus_value(height));
    }
    // sticky_tops
    auto sticky_tops_array = lepus::CArray::Create();
    for (int32_t index : sticky_tops) {
      sticky_tops_array->push_back(lepus_value(index));
    }
    // sticky_bottoms
    auto sticky_bottoms_array = lepus::CArray::Create();
    for (int32_t index : sticky_bottoms) {
      sticky_bottoms_array->push_back(lepus_value(index));
    }
    // full_spans
    auto full_spans_array = lepus::CArray::Create();
    for (int32_t index : full_spans) {
      full_spans_array->push_back(lepus_value(index));
    }
    // insertions
    auto insertion_array = lepus::CArray::Create();
    for (const auto& index : insertion) {
      insertion_array->push_back(lepus_value(index));
    }
    // removals
    auto removal_array = lepus::CArray::Create();
    for (const auto& index : removal) {
      removal_array->push_back(lepus_value(index));
    }
    // update_from
    auto update_from_array = lepus::CArray::Create();
    for (const auto& index : update_from) {
      update_from_array->push_back(lepus_value(index));
    }
    // update_to
    auto update_to_array = lepus::CArray::Create();
    for (const auto& index : update_to) {
      update_to_array->push_back(lepus_value(index));
    }
    // construct diff info.
    auto diff_info = lepus::Dictionary::Create();
    diff_info->SetValue(list::kInsertions, lepus_value(insertion_array));
    diff_info->SetValue(list::kRemovals, lepus_value(removal_array));
    diff_info->SetValue(list::kUpdateFrom, lepus_value(update_from_array));
    diff_info->SetValue(list::kUpdateTo, lepus_value(update_to_array));
    // construct diff result.
    auto diff_result = lepus::Dictionary::Create();
    diff_result->SetValue(list::kDiffResult, lepus_value(diff_info));
    diff_result->SetValue(list::kDataSourceItemKeys,
                          lepus_value(item_keys_array));
    diff_result->SetValue(list::kDataSourceEstimatedHeightPx,
                          lepus_value(estimated_height_px_array));
    diff_result->SetValue(list::kDataSourceEstimatedMainAxisSizePx,
                          lepus_value(estimated_main_axis_px_array));
    diff_result->SetValue(list::kDataSourceFullSpan,
                          lepus_value(full_spans_array));
    diff_result->SetValue(list::kDataSourceStickyTop,
                          lepus_value(sticky_tops_array));
    diff_result->SetValue(list::kDataSourceStickyBottom,
                          lepus_value(sticky_bottoms_array));
    return diff_result;
  }
};

}  // namespace list
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_TESTING_MOCK_DIFF_RESULT_H_
