// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_PUBLIC_LAYOUT_NODE_VALUE_H_
#define CORE_PUBLIC_LAYOUT_NODE_VALUE_H_
#include <memory>

namespace lynx {
namespace tasm {

struct LayoutResult {
  LayoutResult() : width_(0.f), height_(0.f), baseline_(0.f) {}
  LayoutResult(float width, float height)
      : width_(width), height_(height), baseline_(0.f) {}
  LayoutResult(float width, float height, float baseline)
      : width_(width), height_(height), baseline_(baseline) {}
  float width_;
  float height_;
  float baseline_;
};

enum class FlexDirection : unsigned {
  kColumn = 0,
  kRow = 1,
  kRowReverse = 2,
  kColumnReverse = 3,
};

struct LayoutNodeStyle {
  static constexpr float UNDEFINED_MIN_SIZE = 0.f;
  static constexpr float UNDEFINED_MAX_SIZE = static_cast<float>(0x7FFFFFF);
};

// FIXME(zhixuan): Layout type flags is a mess now, we should refactor it
enum LayoutNodeType {
  // Default is UNKNOWN, indicating that the LayoutNodeType corresponding to the
  // current tag is still unknown.
  UNKNOWN = 0,
  // Common node will not have corresponding platform layout node
  COMMON = 1,
  // The layout of virtual node will be handle by its parent which has custom
  // layout instead of layout engine
  VIRTUAL = 1 << 1,
  // Node has custom layout
  CUSTOM = 1 << 2,
  // Node is inline and should be measured by native
  INLINE = 1 << 5
};

class MeasureFunc {
 public:
  virtual ~MeasureFunc() = default;
  virtual LayoutResult Measure(float width, int32_t width_mode, float height,
                               int32_t height_mode, bool final_measure) = 0;
  virtual void Alignment() = 0;
};

}  // namespace tasm

}  // namespace lynx
#endif  // CORE_PUBLIC_LAYOUT_NODE_VALUE_H_
