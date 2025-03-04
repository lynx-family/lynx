// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_SWITCH_H_
#define CORE_RUNTIME_VM_LEPUS_SWITCH_H_

#include <algorithm>
#include <climits>
#include <memory>
#include <utility>
#include <vector>

#include "core/runtime/vm/lepus/token.h"

namespace lynx {
namespace lepus {
enum SwitchType {
  SwitchType_Table,
  SwitchType_Lookup,
};

class Value;
class SwitchInfo {
 public:
  SwitchInfo()
      : key_type_(0),
        min_(LONG_MAX),
        max_(0),
        default_offset_(-1),
        switch_table_() {}

  SwitchInfo(long key_type, long min, long max, long default_offset,
             SwitchType type, std::vector<std::pair<long, long>> switch_table) {
    key_type_ = key_type;
    min_ = min;
    max_ = max;
    default_offset_ = default_offset;
    type_ = type;
    switch_table_ = std::move(switch_table);
  }

  void Modify(Token& key, long offset);
  long BinarySearchTable(long key);
  static bool SortTable(const std::pair<int, int>& v1,
                        const std::pair<int, int>& v2);
  void Adjust();
  long Switch(Value* value);

  long default_offset() const { return default_offset_; }

  long get_key_type() const { return key_type_; }

  long get_min() const { return min_; }

  long get_max() const { return max_; }

  long get_default_offset() const { return default_offset_; }

  std::vector<std::pair<long, long>> get_switch_table() {
    return switch_table_;
  }

  long get_switch_type() { return type_; }

  void set_default_offset(long offset) { default_offset_ = offset; }

 private:
  SwitchType type_;
  long key_type_;
  long min_;
  long max_;
  long default_offset_;
  std::vector<std::pair<long, long>> switch_table_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_SWITCH_H_
