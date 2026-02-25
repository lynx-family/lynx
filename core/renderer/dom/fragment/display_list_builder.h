// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_BUILDER_H_
#define CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_BUILDER_H_

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

namespace lynx {
namespace starlight {
class BordersData;
}
namespace transforms {
class Matrix44;
}
namespace tasm {

class DisplayListBuilder {
 public:
  explicit DisplayListBuilder(float dx = 0, float dy = 0);
  ~DisplayListBuilder();

  DisplayListBuilder(const DisplayListBuilder&) = delete;
  DisplayListBuilder& operator=(const DisplayListBuilder&) = delete;
  DisplayListBuilder(DisplayListBuilder&&) = default;
  DisplayListBuilder& operator=(DisplayListBuilder&&) = default;

  void Reserve(int32_t capacity);

  // Begin a new fragment
  DisplayListBuilder& Begin(int id, float x, float y, float width,
                            float height);

  // End the current fragment
  DisplayListBuilder& End();

  // Fill with color
  DisplayListBuilder& Fill(uint32_t color, int32_t clip_index = -1);

  // Draw a view
  DisplayListBuilder& DrawView(int view_id);

  // Apply transform
  DisplayListBuilder& Transform(const transforms::Matrix44& matrix);
  DisplayListBuilder& Opacity(float alpha);

  // Retrieve Image source and draw
  DisplayListBuilder& DrawImage(int32_t image_id, int32_t box_index);

  // Retrieve text source and draw
  DisplayListBuilder& DrawText(int text_id);

  // Set all border properties at once (color, width, style for all four sides)
  DisplayListBuilder& Border(int32_t out_index, int32_t inner_index,
                             const starlight::BordersData& border);

  // Set clip rect
  DisplayListBuilder& ClipRect(const RoundedRectangle& border);

  // Record box model
  DisplayListBuilder& RecordBoxModel(const RoundedRectangle& rect,
                                     int32_t& index);

  // Draw linear gradient
  DisplayListBuilder& LinearGradient(float angle,
                                     const base::Vector<uint32_t>& colors,
                                     const base::Vector<float>& stops,
                                     int32_t tiling_index, int32_t clip_index,
                                     int32_t repeat_x, int32_t repeat_y);

  DisplayListBuilder& MarkRootNeedClipBounds();

  // Build the final display list
  DisplayList Build();

  // Clear all items
  void Clear();

 private:
  DisplayList display_list_;

  int32_t current_index_of_box_model = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_BUILDER_H_
