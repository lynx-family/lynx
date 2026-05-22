// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LYNX_LIST_DATA_H_
#define CLAY_UI_COMPONENT_LIST_LYNX_LIST_DATA_H_

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace clay {

struct ListNoDiffInfo {
  struct Action {
    std::string item_key;
    std::string type;
    bool full_span;
    bool sticky_top;
    bool sticky_bottom;
    int estimated_height_px;
  };
  struct UpdateAction : Action {
    int from;
    int to;
    bool is_flush;
  };
  struct InsertAction : Action {
    int position;
  };

  std::vector<UpdateAction> update_actions;
  std::vector<int> remove_actions;
  std::vector<InsertAction> insert_actions;
};

class LynxListData {
 public:
  explicit LynxListData(int count);
  ~LynxListData();

  void PushViewType(const char* name);

  const std::vector<int>& GetViewTypes() const { return view_types_; }
  const std::vector<std::string>& GetViewTypeNames() const {
    return view_type_names_;
  }

  void SetNewArch(bool new_arch) { new_arch_ = new_arch; }
  void SetDiffable(bool diffable) { diffable_ = diffable; }
  void SetFullSpan(const uint32_t* indices, int size);
  void SetStickyTop(const uint32_t* indices, int size);
  void SetStickyBottom(const uint32_t* indices, int size);
  void SetInsertions(const int32_t* indices, int size);
  void SetRemovals(const int32_t* indices, int size);
  void SetUpdateFrom(const int32_t* indices, int size);
  void SetUpdateTo(const int32_t* indices, int size);
  void SetMoveFrom(const int32_t* indices, int size);
  void SetMoveTo(const int32_t* indices, int size);

  void SetFullSpan(const std::vector<int32_t>& indices) {
    full_span_.clear();
    full_span_.insert(full_span_.end(), indices.begin(), indices.end());
    std::sort(full_span_.begin(), full_span_.end());
  }

  void SetStickyTop(const std::vector<int32_t>& indices) {
    sticky_top_.clear();
    sticky_top_.insert(sticky_top_.end(), indices.begin(), indices.end());
    std::sort(sticky_top_.begin(), sticky_top_.end());
  }

  void SetStickyBottom(const std::vector<int32_t>& indices) {
    sticky_bottom_.clear();
    sticky_bottom_.insert(sticky_bottom_.end(), indices.begin(), indices.end());
    std::sort(sticky_bottom_.begin(), sticky_bottom_.end());
  }

  void SetInsertions(const std::vector<int32_t>& indices) {
    insertions_.clear();
    insertions_.insert(insertions_.end(), indices.begin(), indices.end());
  }

  void SetRemovals(const std::vector<int32_t>& indices) {
    removals_.clear();
    removals_.insert(removals_.end(), indices.begin(), indices.end());
  }

  void SetUpdateFrom(const std::vector<int32_t>& indices) {
    update_from_.clear();
    update_from_.insert(update_from_.end(), indices.begin(), indices.end());
  }

  void SetUpdateTo(const std::vector<int32_t>& indices) {
    update_to_.clear();
    update_to_.insert(update_to_.end(), indices.begin(), indices.end());
  }

  void SetMoveFrom(const std::vector<int32_t>& indices) {
    move_from_.clear();
    move_from_.insert(move_from_.end(), indices.begin(), indices.end());
  }

  void SetMoveTo(const std::vector<int32_t>& indices) {
    move_to_.clear();
    move_to_.insert(move_to_.end(), indices.begin(), indices.end());
  }

  bool GetNewArch() const { return new_arch_; }
  bool GetDiffable() const { return diffable_; }
  const std::vector<int32_t>& GetFullSpan() const { return full_span_; }
  const std::vector<int32_t>& GetStickyTop() const { return sticky_top_; }
  const std::vector<int32_t>& GetStickyBottom() const { return sticky_bottom_; }
  const std::vector<int32_t>& GetInsertions() const { return insertions_; }
  const std::vector<int32_t>& GetRemovals() const { return removals_; }
  const std::vector<int32_t>& GetUpdateFrom() const { return update_from_; }
  const std::vector<int32_t>& GetUpdateTo() const { return update_to_; }
  const std::vector<int32_t>& GetMoveFrom() const { return move_from_; }
  const std::vector<int32_t>& GetMoveTo() const { return move_to_; }

  void DispatchNoDiffActions(ListNoDiffInfo* info);
  void TransformExtraData();

 private:
  int GetViewTypeIndex(const std::string& type_name);
  void InsertViewType(int index, const std::string& type_name);
  void SetViewType(int index, const std::string& type_name);
  void RemoveViewType(int index);

  int count_;
  std::vector<int> view_types_;
  std::vector<std::string> view_type_names_;
  std::unordered_map<std::string, int> registered_;
  bool new_arch_ = false;
  bool diffable_ = false;
  std::vector<int32_t> full_span_;
  std::vector<int32_t> sticky_top_;
  std::vector<int32_t> sticky_bottom_;
  std::vector<int32_t> insertions_;
  std::vector<int32_t> removals_;
  std::vector<int32_t> update_from_;
  std::vector<int32_t> update_to_;
  std::vector<int32_t> move_from_;
  std::vector<int32_t> move_to_;

  std::vector<int32_t> fiber_full_span_;
  std::vector<int32_t> fiber_sticky_top_;
  std::vector<int32_t> fiber_sticky_bottom_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LYNX_LIST_DATA_H_
