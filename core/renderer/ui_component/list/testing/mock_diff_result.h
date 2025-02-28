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

class InsertOp {
 public:
  int position_{0};
  std::string item_key_;
  int estimated_main_axis_size_px_{0};
  bool full_span_{false};
  bool sticky_top_{false};
  bool sticky_bottom_{false};

  fml::RefPtr<lepus::Dictionary> ToMap() const {
    auto insert_action = lepus::Dictionary::Create();
    insert_action->SetValue(list::kPosition, lepus::Value(position_));
    insert_action->SetValue(list::kItemKey, lepus::Value(item_key_));
    insert_action->SetValue(list::kEstimatedMainAxisSizePx,
                            lepus::Value(estimated_main_axis_size_px_));
    insert_action->SetValue(list::kFullSpan, lepus::Value(full_span_));
    insert_action->SetValue(list::kStickyTop, lepus::Value(sticky_top_));
    insert_action->SetValue(list::kStickyBottom, lepus::Value(sticky_bottom_));
    return insert_action;
  }
};

class UpdateOp : public InsertOp {
 public:
  int from_{list::kInvalidIndex};
  int to_{list::kInvalidIndex};
  bool flush_{false};
  std::string item_key_;
  int estimated_main_axis_size_px_{0};
  bool full_span_{false};
  bool sticky_top_{false};
  bool sticky_bottom_{false};

  fml::RefPtr<lepus::Dictionary> ToMap() const {
    auto update_action = InsertOp::ToMap();
    update_action->SetValue(list::kFrom, lepus::Value(from_));
    update_action->SetValue(list::kTo, lepus::Value(to_));
    update_action->SetValue(list::kFlush, lepus::Value(flush_));
    update_action->SetValue(list::kItemKey, lepus::Value(item_key_));
    update_action->SetValue(list::kEstimatedMainAxisSizePx,
                            lepus::Value(estimated_main_axis_size_px_));
    update_action->SetValue(list::kFullSpan, lepus::Value(full_span_));
    update_action->SetValue(list::kStickyTop, lepus::Value(sticky_top_));
    update_action->SetValue(list::kStickyBottom, lepus::Value(sticky_bottom_));
    return update_action;
  }
};

class InsertAction {
 public:
  std::vector<InsertOp> insert_ops_;

  fml::RefPtr<lepus::CArray> ToArray() const {
    auto insert_action = lepus::CArray::Create();
    for (const auto& insert_op : insert_ops_) {
      insert_action->emplace_back(insert_op.ToMap());
    }
    return insert_action;
  }
};

class RemoveAction {
 public:
  std::vector<int> remove_ops_;

  fml::RefPtr<lepus::CArray> ToArray() const {
    auto remove_action = lepus::CArray::Create();
    for (const auto& remove_op : remove_ops_) {
      remove_action->emplace_back(lepus::Value(remove_op));
    }
    return remove_action;
  }
};

class UpdateAction {
 public:
  std::vector<UpdateOp> update_ops_;

  fml::RefPtr<lepus::CArray> ToArray() const {
    auto update_action = lepus::CArray::Create();
    for (const auto& update_op : update_ops_) {
      update_action->emplace_back(update_op.ToMap());
    }
    return update_action;
  }
};

class FiberDiffResult {
 public:
  InsertAction insert_action_;
  RemoveAction remove_action_;
  UpdateAction update_action_;

  fml::RefPtr<lepus::Dictionary> Resolve() const {
    auto diff_result = lepus::Dictionary::Create();
    diff_result->SetValue(list::kFiberInsertAction, insert_action_.ToArray());
    diff_result->SetValue(list::kFiberRemoveAction, remove_action_.ToArray());
    diff_result->SetValue(list::kFiberUpdateAction, update_action_.ToArray());
    return diff_result;
  }
};

}  // namespace list
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_TESTING_MOCK_DIFF_RESULT_H_
