// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_MACROS_H_
#define CLAY_UI_GESTURE_MACROS_H_

namespace clay {

#if (DEBUG_GESTURE)
#define GESTURE_LOG FML_LOG(ERROR) << "[Gesture] "
#else
#define GESTURE_LOG FML_EAT_STREAM_PARAMETERS(true)
#endif

}  // namespace clay

#endif  // CLAY_UI_GESTURE_MACROS_H_
