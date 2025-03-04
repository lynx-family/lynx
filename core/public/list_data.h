// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LIST_DATA_H_
#define CORE_PUBLIC_LIST_DATA_H_

#include <algorithm>
#include <string>
#include <vector>

namespace lynx {
namespace tasm {
/*
 * `ListData` provides the necessary information for lazy-loading of the
 * `<list>`, such as diff, full-span, sticky, etc. This information is generated
 * by the FE framework. It has been deprecated under Fiber.
 */
class ListData {
 public:
  void SetViewTypeNames(const std::vector<std::string>& names) {
    for (const std::string& name : names) {
      view_type_names_.emplace_back(name.c_str());
    }
  }

  void SetNewArch(bool new_arch) { new_arch_ = new_arch; }

  void SetDiffable(bool diffable) { diffable_ = diffable; }

  void SetFullSpan(const std::vector<uint32_t>& indices) {
    full_span_.clear();
    full_span_.insert(full_span_.end(), indices.begin(), indices.end());
    std::sort(full_span_.begin(), full_span_.end());
  }

  void SetStickyTop(const std::vector<uint32_t>& indices) {
    sticky_top_.clear();
    sticky_top_.insert(sticky_top_.end(), indices.begin(), indices.end());
    std::sort(sticky_top_.begin(), sticky_top_.end());
  }

  void SetStickyBottom(const std::vector<uint32_t>& indices) {
    sticky_bottom_.clear();
    sticky_bottom_.insert(sticky_bottom_.end(), indices.begin(), indices.end());
    std::sort(sticky_bottom_.begin(), sticky_bottom_.end());
  }

  template <class VECTOR_LIKE>
  void SetInsertions(const VECTOR_LIKE& indices) {
    insertions_.clear();
    insertions_.insert(insertions_.end(), indices.begin(), indices.end());
  }

  template <class VECTOR_LIKE>
  void SetRemovals(const VECTOR_LIKE& indices) {
    removals_.clear();
    removals_.insert(removals_.end(), indices.begin(), indices.end());
  }

  template <class VECTOR_LIKE>
  void SetUpdateFrom(const VECTOR_LIKE& indices) {
    update_from_.clear();
    update_from_.insert(update_from_.end(), indices.begin(), indices.end());
  }

  template <class VECTOR_LIKE>
  void SetUpdateTo(const VECTOR_LIKE& indices) {
    update_to_.clear();
    update_to_.insert(update_to_.end(), indices.begin(), indices.end());
  }

  template <class VECTOR_LIKE>
  void SetMoveFrom(const VECTOR_LIKE& indices) {
    move_from_.clear();
    move_from_.insert(move_from_.end(), indices.begin(), indices.end());
  }

  template <class VECTOR_LIKE>
  void SetMoveTo(const VECTOR_LIKE& indices) {
    move_to_.clear();
    move_to_.insert(move_to_.end(), indices.begin(), indices.end());
  }

  const std::vector<std::string>& GetViewTypeNames() const {
    return view_type_names_;
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

 private:
  std::vector<std::string> view_type_names_;
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
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_LIST_DATA_H_
