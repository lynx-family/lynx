// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_PUBLIC_LAYOUT_NODE_MANAGER_H_
#define CORE_PUBLIC_LAYOUT_NODE_MANAGER_H_

#include <memory>

#include "core/public/layout_node_value.h"

namespace lynx {

namespace tasm {

class LayoutNodeManager {
 public:
  LayoutNodeManager() = default;
  virtual ~LayoutNodeManager() = default;
  virtual void SetMeasureFunc(int32_t id,
                              std::unique_ptr<MeasureFunc> measure_func) = 0;
  virtual void MarkDirtyAndRequestLayout(int32_t id) = 0;
  virtual void MarkDirtyAndForceLayout(int32_t id) = 0;
  virtual bool IsDirty(int32_t id) = 0;
  virtual FlexDirection GetFlexDirection(int32_t id) = 0;
  virtual float GetWidth(int32_t id) = 0;
  virtual float GetHeight(int32_t id) = 0;
  virtual float GetMinWidth(int32_t id) = 0;
  virtual float GetMaxWidth(int32_t id) = 0;
  virtual float GetMinHeight(int32_t id) = 0;
  virtual float GetMaxHeight(int32_t id) = 0;
  virtual float GetPaddingLeft(int32_t id) = 0;
  virtual float GetPaddingTop(int32_t id) = 0;
  virtual float GetPaddingRight(int32_t id) = 0;
  virtual float GetPaddingBottom(int32_t id) = 0;
  virtual float GetMarginLeft(int32_t id) = 0;
  virtual float GetMarginTop(int32_t id) = 0;
  virtual float GetMarginRight(int32_t id) = 0;
  virtual float GetMarginBottom(int32_t id) = 0;
  virtual LayoutResult UpdateMeasureByPlatform(int32_t id, float width,
                                               int32_t width_mode, float height,
                                               int32_t height_mode,
                                               bool final_measure) = 0;
  virtual void AlignmentByPlatform(int32_t id, float offset_top,
                                   float offset_left) = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_LAYOUT_NODE_MANAGER_H_
