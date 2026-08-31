// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_

#include <native_drawing/drawing_canvas.h>

#include <memory>
#include <vector>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxRendererContext;
class BackgroundDrawable;
class UIBase;

class LynxDisplayListApplier {
 public:
  LynxDisplayListApplier(LynxRendererContext* context,
                         std::weak_ptr<UIBase> host);
  ~LynxDisplayListApplier();

  void ApplyDisplayList(const DisplayList& display_list,
                        OH_Drawing_Canvas* canvas);

 private:
  void ProcessContentOperations(const DisplayListItem* items, size_t item_count,
                                OH_Drawing_Canvas* canvas, float density);

  LynxRendererContext* context_{nullptr};
  std::weak_ptr<UIBase> host_;
  std::vector<RoundedRectangle> boxes_;
  std::unique_ptr<BackgroundDrawable> fill_drawable_;
  std::unique_ptr<BackgroundDrawable> border_drawable_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_
