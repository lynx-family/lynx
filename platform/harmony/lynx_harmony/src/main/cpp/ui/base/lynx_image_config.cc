// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_config.h"

namespace lynx {
namespace tasm {
namespace harmony {
void LynxImageConfig::SetEnableImageLoadCallback(bool enable) {
  enable_image_load_callback_ = enable;
}

bool LynxImageConfig::GetEnableImageLoadCallback() {
  return enable_image_load_callback_;
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
