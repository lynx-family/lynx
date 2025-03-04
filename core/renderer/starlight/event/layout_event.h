// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_H_
#define CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_H_

namespace lynx {
namespace starlight {

enum class LayoutEventType {
  UpdateMeasureBegin,
  UpdateMeasureEnd,
  UpdateAlignmentBegin,
  UpdateAlignmentEnd,
  RemoveAlgorithmRecursiveBegin,
  RemoveAlgorithmRecursiveEnd,
  RoundToPixelGridBegin,
  RoundToPixelGridEnd,
  LayoutStyleError,
  FeatureCountOnGridDisplay,
  FeatureCountOnRelativeDisplay,
};

}
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_EVENT_LAYOUT_EVENT_H_
