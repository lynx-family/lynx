// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_MODIFIER_SCROLL_STATE_H_
#define CORE_RENDERER_DOM_FIBER_MODIFIER_SCROLL_STATE_H_

#include <algorithm>
#include <cstdint>

namespace lynx {
namespace tasm {

// Distances are layout units. Layout corrections are never charged to a
// subsequent drag, and fractional offsets survive platform pixel rounding.
class ModifierScrollState {
 public:
  void Initialize(double offset) { offset_ = std::max(0., offset); }
  void Invalidate() { measured_ = false; }

  bool UpdateLayout(float viewport, float content) {
    const bool changed =
        !measured_ || viewport_ != viewport || content_ != content;
    viewport_ = viewport;
    content_ = content;
    measured_ = true;
    offset_ = std::clamp(offset_, 0., max_offset());
    if (changed) ++revision_;
    return changed;
  }

  double ScrollTo(double target) {
    if (!measured_) return 0;
    const double previous = offset_;
    offset_ = std::clamp(target, 0., max_offset());
    if (offset_ != previous) ++revision_;
    return offset_ - previous;
  }

  float physical_offset(bool reverse) const {
    return reverse ? max_offset() - offset_ : offset_;
  }
  double max_offset() const {
    return std::max(0., double(content_) - viewport_);
  }
  double offset() const { return offset_; }
  float viewport() const { return viewport_; }
  float content() const { return content_; }
  bool measured() const { return measured_; }
  uint64_t revision() const { return revision_; }

 private:
  bool measured_{false};
  double offset_{0};
  float viewport_{0};
  float content_{0};
  uint64_t revision_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_MODIFIER_SCROLL_STATE_H_
