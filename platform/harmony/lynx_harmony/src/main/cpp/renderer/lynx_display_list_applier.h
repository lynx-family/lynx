// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_DISPLAY_LIST_APPLIER_H_

#include <native_drawing/drawing_canvas.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/display_list_reader.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxRendererContext;
class BackgroundDrawable;
class UIBase;

struct DisplayListSegment {
  size_t start_item_index{0};
  size_t end_item_index{0};
  int32_t preceding_view_id{-1};

  // Active saves/translations and clips at the beginning of this segment.
  base::Vector<size_t> state_item_indices;
  bool has_drawing{false};

  size_t ItemCount() const { return end_item_index - start_item_index; }
  bool IsEmpty() const { return start_item_index == end_item_index; }
};

class LynxDisplayListApplier {
 public:
  LynxDisplayListApplier(LynxRendererContext* context,
                         std::weak_ptr<UIBase> host);
  ~LynxDisplayListApplier();

  base::Vector<DisplayListSegment> UpdateDisplayList(
      const DisplayList& display_list);
  void ApplyDisplayList(const DisplayList& display_list,
                        OH_Drawing_Canvas* canvas,
                        const DisplayListSegment& segment);

 private:
  void ProcessContentOperations(const DisplayListItem* items,
                                const DisplayListReader& reader,
                                OH_Drawing_Canvas* canvas, float density,
                                const DisplayListSegment& segment);
  void DrawBackgroundImage(OH_Drawing_Canvas* canvas, int32_t image_id,
                           int32_t tiling_index, int32_t clip_index,
                           int32_t repeat_x, int32_t repeat_y,
                           int32_t auto_size, float position_x,
                           float position_y, float density);

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
