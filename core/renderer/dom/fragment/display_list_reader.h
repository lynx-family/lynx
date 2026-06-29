// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_READER_H_
#define CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_READER_H_

#include <cstddef>
#include <cstdint>

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx {
namespace tasm {

class DisplayListReader {
 public:
  DisplayListReader()
      : items_(nullptr), data_(nullptr), item_count_(0), item_index_(0) {}

  explicit DisplayListReader(const DisplayList& list);

  bool HasNext() const { return item_index_ < item_count_; }

  const DisplayListItem& Next() {
    const DisplayListItem& item = *reinterpret_cast<const DisplayListItem*>(
        items_ + item_index_ * sizeof(DisplayListItem));
    item_index_++;
    return item;
  }

  const uint32_t* Colors(const DisplayListItem& item) const {
    if (item.payload.linear_gradient.color_count == 0) {
      return nullptr;
    }
    return reinterpret_cast<const uint32_t*>(
        data_ + item.payload.linear_gradient.color_count_offset);
  }

  const float* Stops(const DisplayListItem& item) const {
    if (item.payload.linear_gradient.stop_count == 0) {
      return nullptr;
    }
    return reinterpret_cast<const float*>(
        data_ + item.payload.linear_gradient.stop_count_offset);
  }

  size_t Remaining() const { return item_count_ - item_index_; }

  void Reset() { item_index_ = 0; }

 private:
  const uint8_t* items_;
  const uint8_t* data_;
  size_t item_count_;
  size_t item_index_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_READER_H_
