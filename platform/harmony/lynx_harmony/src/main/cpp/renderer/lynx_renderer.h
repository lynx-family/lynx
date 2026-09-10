// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_H_

#include <native_drawing/drawing_canvas.h>

#include <cstdint>
#include <memory>

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxDisplayListApplier;
class LynxRendererContext;
class UIBase;

class LynxRenderer {
 public:
  LynxRenderer(std::shared_ptr<LynxRendererContext> context, int32_t sign,
               std::weak_ptr<UIBase> host);
  ~LynxRenderer();
  int32_t Sign() const { return sign_; }
  void UpdateDisplayList(DisplayList display_list);
  void Draw(OH_Drawing_Canvas* canvas);

 private:
  std::shared_ptr<LynxRendererContext> context_;
  std::weak_ptr<UIBase> host_;
  int32_t sign_{0};
  std::unique_ptr<LynxDisplayListApplier> display_list_applier_;
  DisplayList display_list_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_H_
