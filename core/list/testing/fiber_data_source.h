// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_FIBER_DATA_SOURCE_H_
#define CORE_LIST_TESTING_FIBER_DATA_SOURCE_H_

#include <string>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {
namespace testing {

class InsertOp {
 public:
  int position_{0};
  std::string item_key_;
  int estimated_main_axis_size_px_{0};
  bool full_span_{false};
  bool sticky_top_{false};
  bool sticky_bottom_{false};
  bool recyclable_{true};

  fml::RefPtr<lepus::Dictionary> ToMap() const {
    auto insert_action = lepus::Dictionary::Create();
    insert_action->SetValue(kFiberDataPosition, lepus::Value(position_));
    insert_action->SetValue(kFiberDataItemKey, lepus::Value(item_key_));
    insert_action->SetValue(kFiberDataEstimatedMainAxisSizePx,
                            lepus::Value(estimated_main_axis_size_px_));
    insert_action->SetValue(kFiberDataFullSpan, lepus::Value(full_span_));
    insert_action->SetValue(kFiberDataStickyTop, lepus::Value(sticky_top_));
    insert_action->SetValue(kFiberDataStickyBottom,
                            lepus::Value(sticky_bottom_));
    insert_action->SetValue(kFiberDataRecyclable, lepus::Value(recyclable_));
    return insert_action;
  }
};

class UpdateOp {
 public:
  int from_{kInvalidIndex};
  int to_{kInvalidIndex};
  bool flush_{false};
  std::string item_key_;
  int estimated_main_axis_size_px_{0};
  bool full_span_{false};
  bool sticky_top_{false};
  bool sticky_bottom_{false};
  bool recyclable_{true};

  fml::RefPtr<lepus::Dictionary> ToMap() const {
    auto update_action = lepus::Dictionary::Create();
    update_action->SetValue(kFiberDataFrom, lepus::Value(from_));
    update_action->SetValue(kFiberDataTo, lepus::Value(to_));
    update_action->SetValue(kFiberDataFlush, lepus::Value(flush_));
    update_action->SetValue(kFiberDataItemKey, lepus::Value(item_key_));
    update_action->SetValue(kFiberDataEstimatedMainAxisSizePx,
                            lepus::Value(estimated_main_axis_size_px_));
    update_action->SetValue(kFiberDataFullSpan, lepus::Value(full_span_));
    update_action->SetValue(kFiberDataStickyTop, lepus::Value(sticky_top_));
    update_action->SetValue(kFiberDataStickyBottom,
                            lepus::Value(sticky_bottom_));
    update_action->SetValue(kFiberDataRecyclable, lepus::Value(recyclable_));
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

class FiberDataSource {
 public:
  InsertAction insert_action_;
  RemoveAction remove_action_;
  UpdateAction update_action_;

  fml::RefPtr<lepus::Dictionary> Resolve() const {
    auto diff_result = lepus::Dictionary::Create();
    diff_result->SetValue(kFiberDataInsertAction, insert_action_.ToArray());
    diff_result->SetValue(kFiberDataRemoveAction, remove_action_.ToArray());
    diff_result->SetValue(kFiberDataUpdateAction, update_action_.ToArray());
    return diff_result;
  }
};

}  // namespace testing
}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_FIBER_DATA_SOURCE_H_
