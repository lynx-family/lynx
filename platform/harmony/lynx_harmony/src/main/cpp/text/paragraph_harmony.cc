// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {

void ParagraphHarmony::Draw(OH_Drawing_Canvas* canvas, const double x,
                            const double y) const {
  OH_Drawing_TypographyPaint(paragraph_, canvas, x, y);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
