// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <Lynx/LynxLayoutNode.h>

#include "core/public/layout_node_value.h"

namespace lynx {
namespace tasm {

class LynxMeasureFuncDarwin : public MeasureFunc {
 public:
  LynxMeasureFuncDarwin(LynxLayoutNode* layoutNode);
  ~LynxMeasureFuncDarwin() override = default;
  LayoutResult Measure(float width, int width_mode, float height,
                       int height_mode, bool final_measure) override;
  void Alignment() override;

 private:
  LynxLayoutNode* shadowNode;
};

}  // namespace tasm
}  // namespace lynx
