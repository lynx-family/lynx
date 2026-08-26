// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_

#include <native_drawing/drawing_canvas.h>

#include <memory>
#include <vector>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/display_list_segment.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"

namespace lynx {
namespace tasm {
namespace harmony {

class BackgroundDrawable;
class UIBase;

class LynxDisplayListApplier {
 public:
  explicit LynxDisplayListApplier(LynxRendererContext* context);
  ~LynxDisplayListApplier();

  void UpdateDisplayListResources(const DisplayList& display_list);
  void DrawSegment(const DisplayList& display_list,
                   const DisplayListSegmentResult& segment_result,
                   const DisplayListSegment& segment,
                   const std::shared_ptr<UIBase>& renderer_host,
                   OH_Drawing_Canvas* canvas);
  void Reset();

 private:
  void ProcessContentOperations(const DisplayList& display_list,
                                const DisplayListSegmentResult& segment_result,
                                const DisplayListSegment& segment,
                                const std::shared_ptr<UIBase>& renderer_host,
                                OH_Drawing_Canvas* canvas,
                                bool skip_first_translate);

  LynxRendererContext* context_{nullptr};
  std::vector<RoundedRectangle> boxes_;
  std::vector<size_t> replay_item_indices_;
  std::unique_ptr<BackgroundDrawable> fill_drawable_;
  std::unique_ptr<BackgroundDrawable> border_drawable_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_
