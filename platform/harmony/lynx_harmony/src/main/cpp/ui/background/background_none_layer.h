// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_NONE_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_NONE_LAYER_H_

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_layer.h"

namespace lynx {
namespace tasm {
namespace harmony {
class BackgroundNoneLayer : public BackgroundLayer {
  bool IsReady() override;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_NONE_LAYER_H_
