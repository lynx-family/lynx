// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_TEXT_SELECTION_H_
#define CLAY_UI_COMMON_TEXT_SELECTION_H_

#include <algorithm>

#include "clay/ui/common/editing_misc.h"

namespace clay {

// Represents cursor selection of range in text.
class TextSelection {
 public:
  TextSelection() : base_offset_(0), extent_offset_(0) {}

  explicit TextSelection(int same_offset)
      : base_offset_(same_offset), extent_offset_(same_offset) {}

  TextSelection(int base_offset, int extent_offset, Affinity affinity)
      : base_offset_(base_offset),
        extent_offset_(extent_offset),
        affinity_(affinity) {}

  void ResetToStart() { MoveTo(0, 0); }

  void MoveLeft(int step) { Move(-step); }

  void MoveRight(int step) { Move(step); }

  // Move caret without selection.
  void Move(int step) {
    // Here we don't use |base_offset_| because there's no selection.
    MoveTo(extent_offset_ + step, extent_offset_ + step);
  }

  void MoveExtent(int step) { extent_offset_ += step; }

  void MoveTo(int base_offset, int extent_offset) {
    base_offset_ = base_offset;
    extent_offset_ = extent_offset;
  }

  bool IsValid() const { return base_offset_ >= 0 && extent_offset_ >= 0; }

  int base_offset() const { return base_offset_; }
  int extent_offset() const { return extent_offset_; }
  int start() const { return std::min(base_offset_, extent_offset_); }
  int end() const { return std::max(base_offset_, extent_offset_); }
  bool collapsed() const { return base_offset_ == extent_offset_; }
  Affinity affinity() const { return affinity_; }

  void SetBaseOffset(int base_offset) { base_offset_ = base_offset; }
  void SetExtentOffset(int extent_offset) { extent_offset_ = extent_offset; }
  void SetSameOffset(int same_offset) {
    base_offset_ = same_offset;
    extent_offset_ = same_offset;
  }
  void set_affinity(Affinity value) { affinity_ = value; }

 private:
  // TODO(yulitao): consider when need offset be -1.
  int base_offset_ = -1;
  // Extent offset is always where caret is.
  int extent_offset_ = -1;
  Affinity affinity_ = Affinity::kDownstream;
};

inline bool operator==(const TextSelection& lhs, const TextSelection& rhs) {
  return lhs.base_offset() == rhs.base_offset() &&
         lhs.extent_offset() == rhs.extent_offset() &&
         lhs.affinity() == rhs.affinity();
}

inline bool operator!=(const TextSelection& lhs, const TextSelection& rhs) {
  return !(lhs == rhs);
}

}  // namespace clay

#endif  // CLAY_UI_COMMON_TEXT_SELECTION_H_
