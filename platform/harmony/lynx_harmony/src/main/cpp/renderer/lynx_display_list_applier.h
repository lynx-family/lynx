// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_

#include <native_drawing/drawing_canvas.h>

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxRendererContext;

class LynxDisplayListApplier {
 public:
  explicit LynxDisplayListApplier(LynxRendererContext* context);
  ~LynxDisplayListApplier();

  void ApplyDisplayList(const DisplayList& display_list,
                        OH_Drawing_Canvas* canvas);

 private:
  void ProcessContentOperations(const DisplayListItem* items, size_t item_count,
                                OH_Drawing_Canvas* canvas, float density);

  LynxRendererContext* context_{nullptr};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_
