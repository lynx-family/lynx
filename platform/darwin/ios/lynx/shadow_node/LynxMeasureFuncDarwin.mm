// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxMeasureFuncDarwin.h"

namespace lynx {
namespace tasm {

LynxMeasureFuncDarwin::LynxMeasureFuncDarwin(LynxLayoutNode* node) : shadowNode(node) {}

LayoutResult LynxMeasureFuncDarwin::Measure(float width, int width_mode, float height,
                                            int height_mode, bool final_measure) {
  LynxMeasureMode widthMode = (LynxMeasureMode)width_mode;
  LynxMeasureMode heightMode = (LynxMeasureMode)height_mode;
  MeasureResult result = [shadowNode measureWithWidth:width
                                            widthMode:widthMode
                                               height:height
                                           heightMode:heightMode
                                         finalMeasure:final_measure];
  return LayoutResult{(float)result.size.width, (float)result.size.height, (float)result.baseline};
}
void LynxMeasureFuncDarwin::Alignment() { [shadowNode align]; }

}  // namespace tasm
}  // namespace lynx
