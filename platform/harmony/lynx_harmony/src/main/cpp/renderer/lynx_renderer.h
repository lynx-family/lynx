// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_H_

#include <memory>
#include <vector>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/display_list_segment.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIBase;
class LynxRenderNode;

class LynxRenderer {
 public:
  LynxRenderer(std::shared_ptr<LynxRendererContext> context, int32_t sign);
  ~LynxRenderer();

  int32_t Sign() const { return sign_; }
  void UpdateDisplayList(DisplayList&& display_list,
                         const std::shared_ptr<UIBase>& host);
  void Draw(OH_Drawing_Canvas* canvas, const std::shared_ptr<UIBase>& host);
  void DrawSegment(size_t segment_index, OH_Drawing_Canvas* canvas);
  void UpdateRenderNodeOrder(const std::shared_ptr<UIBase>& host);
  void UpdateBounds(float width, float height);
  void InvalidateRenderNodes();

 private:
  void UpdateRenderNodes(const std::shared_ptr<UIBase>& host);

  std::shared_ptr<LynxRendererContext> context_;
  int32_t sign_{0};
  std::unique_ptr<LynxDisplayListApplier> display_list_applier_;
  DisplayList display_list_;
  DisplayListSegmentResult segment_result_;
  std::vector<std::unique_ptr<LynxRenderNode>> render_nodes_;
  std::weak_ptr<UIBase> host_;
  float width_{0.f};
  float height_{0.f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_H_
