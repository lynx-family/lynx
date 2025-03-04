// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/switch.h"

#include <algorithm>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace lepus {

bool SwitchInfo::SortTable(const std::pair<int, int>& v1,
                           const std::pair<int, int>& v2) {
  return v1.first < v2.first;
}

long SwitchInfo::BinarySearchTable(long key) {
  if (switch_table_.size() == 0) {
    return -1;
  }
  long low = 0;
  long high = switch_table_.size() - 1;
  long mid = 0;
  while (low <= high) {
    mid = (low + high) / 2;
    if (switch_table_[mid].first < key) {
      low = mid + 1;
    } else if (switch_table_[mid].first > key) {
      high = mid - 1;
    } else {
      return mid;
    }
  }
  return -1;
}

void SwitchInfo::Modify(Token& key, long offset) {
  if (key.token_ == Token_Default) {
    default_offset_ = offset;
    return;
  }
  if (type_ == SwitchType_Table) {
    if (key.token_ != Token_Number || min_ > max_) {
      return;
    }

    long k = static_cast<long>(key.number_);
    if (k < min_ || k > max_) {
      default_offset_ = offset;
      return;
    }
    long index = k - min_;

    switch_table_[index].second = offset;
    return;
  } else if (type_ == SwitchType_Lookup) {
    long key_index;
    if (key.token_ == Token_Number) {
      key_index = key.number_;
    } else if (key.token_ == Token_String) {
      key_index = key.str_.hash();
    } else {
      return;
    }
    long search_index = BinarySearchTable(key_index);
    if (search_index != -1) {
      switch_table_[search_index].second = offset;
    }
  }
}

void SwitchInfo::Adjust() {
  if (type_ != SwitchType_Table) {
    std::sort(switch_table_.begin(), switch_table_.end(), SortTable);
  }
}

long SwitchInfo::Switch(Value* value) {
  if (type_ == SwitchType_Table) {
    if (!value->IsNumber() || min_ > max_) {
      return -1;
    }
    long v = static_cast<long>(value->Number());
    long index = v - min_;
    if (v < min_ || v > max_ || switch_table_[index].second < 0) {
      return default_offset_;
    }
    return switch_table_[index].second;
  } else if (type_ == SwitchType_Lookup) {
    long key_index = 0;
    if (value->IsNumber()) {
      key_index = value->Number();
    } else if (value->IsString()) {
      key_index = value->String().hash();
    }
    long search_index = BinarySearchTable(key_index);
    if (search_index != -1) {
      return switch_table_[search_index].second;
    }
    return default_offset_;
  }
  return -1;
}
}  // namespace lepus
}  // namespace lynx
