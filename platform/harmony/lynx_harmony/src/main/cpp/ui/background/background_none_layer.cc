// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_none_layer.h"

namespace lynx {
namespace tasm {
namespace harmony {

bool BackgroundNoneLayer::IsReady() { return BackgroundLayer::IsReady(); }
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
