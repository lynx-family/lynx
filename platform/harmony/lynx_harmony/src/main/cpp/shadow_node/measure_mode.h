// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_MEASURE_MODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_MEASURE_MODE_H_
#include <cstdint>

namespace lynx {

namespace tasm {

namespace harmony {

enum class MeasureMode : int32_t { Indefinite = 0, Definite = 1, AtMost = 2 };

}
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_MEASURE_MODE_H_
