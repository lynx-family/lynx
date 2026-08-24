// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FRAME_CHILD_DATA_H_
#define CLAY_UI_COMPONENT_FRAME_CHILD_DATA_H_

namespace clay {
class FrameChildDataAndroid;
}  // namespace clay

namespace lynx {
namespace tasm {

#if defined(OS_ANDROID)
using FrameChildData = clay::FrameChildDataAndroid;
#else
class TemplateData;
using FrameChildData = TemplateData;
#endif

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_UI_COMPONENT_FRAME_CHILD_DATA_H_
