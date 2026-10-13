// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_DISPLAY_LIST_BUILDER_H_
#define GFX_PUBLIC_DISPLAY_LIST_BUILDER_H_

#include <array>
#include <cstdint>

#include "core/public/platform_renderer_type.h"
#include "gfx/public/display_list.h"
#include "gfx/public/rounded_rectangle.h"

namespace lynx {
namespace gfx {
class Matrix44;
}  // namespace gfx
namespace tasm {

class PaintImage;

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
  DisplayListBuilder& Begin(int id, PlatformRendererType type, float x, float y,
                            float width, float height);
  // Extended form used by Fragment Layer Renderer to preserve state for
  // PlatformEventTarget hit testing.
  DisplayListBuilder& Begin(int id, PlatformRendererType type, float x, float y,
                            float width, float height, bool overflow_x,
                            bool overflow_y, bool is_layout_only = false);

  // End the current fragment
  DisplayListBuilder& End();

  // Fill with color
  DisplayListBuilder& Fill(uint32_t color, int32_t clip_index = -1);

  // Draw a view at its final position relative to the platform parent.
  DisplayListBuilder& DrawView(int view_id, float offset_x, float offset_y);

  // Apply transform
  DisplayListBuilder& Transform(const gfx::Matrix44& matrix);
  DisplayListBuilder& Opacity(float alpha);
  // Record a protocol filter type and its resolved numeric amount.
  DisplayListBuilder& Filter(int32_t type, float amount);

  // Record an image and retain its reference. Rvalues transfer ownership;
  // lvalues keep the caller's reference and add one for the display list.
  DisplayListBuilder& DrawImage(fml::RefPtr<PaintImage> image,
                                int32_t box_index);

  // Retrieve text source and draw
  DisplayListBuilder& DrawText(int text_id, int32_t box_index);

  // Record border colors/styles in top, right, bottom, left order.
  DisplayListBuilder& Border(int32_t out_index, int32_t inner_index,
                             const std::array<uint32_t, 4>& colors,
                             const std::array<int32_t, 4>& styles);

  // Set clip rect
  DisplayListBuilder& ClipRect(const RoundedRectangle& border);

  // Record box model
  DisplayListBuilder& RecordBoxModel(const RoundedRectangle& rect,
                                     int32_t& index);

  // Draw linear gradient
  // tiling_index: the box index for gradient tiling (size determined by
  // background-origin and background-size)
  // clip_index: the box index for clipping/filling (determined by
  // background-clip). The gradient tiling box is repeated to fill this area.
  // repeat_x, repeat_y: background-repeat values
  DisplayListBuilder& LinearGradient(float angle,
                                     const base::Vector<uint32_t>& colors,
                                     const base::Vector<float>& stops,
                                     int32_t tiling_index, int32_t clip_index,
                                     int32_t repeat_x, int32_t repeat_y);

  DisplayListBuilder& RadialGradient(float center_x, float center_y,
                                     float radius_x, float radius_y,
                                     const base::Vector<uint32_t>& colors,
                                     const base::Vector<float>& stops,
                                     int32_t tiling_index, int32_t clip_index,
                                     int32_t repeat_x, int32_t repeat_y);

  // Image references follow DrawImage's ownership semantics.
  DisplayListBuilder& BackgroundImage(
      fml::RefPtr<PaintImage> image, int32_t tiling_index, int32_t clip_index,
      int32_t repeat_x, int32_t repeat_y, bool auto_width = false,
      bool auto_height = false, float position_x = 0.f, float position_y = 0.f);

  // Box-shadow clip mode enumeration
  // kOutset = 0 (clip to border-box for outside shadows)
  // kInset = 1 (clip to padding-box for inside shadows)
  enum class BoxShadowClipMode : int32_t {
    kOutset = 0,
    kInset = 1,
  };

  // Draw box shadow
  // shadow_box_index: pre-computed shadow box with adjusted rect/radii
  // clip_box_index: original box for inset clip (padding box for inset, border
  // box for outset)
  DisplayListBuilder& BoxShadow(int32_t shadow_box_index,
                                int32_t clip_box_index, uint32_t color,
                                float blur_radius, BoxShadowClipMode clip_mode);

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

#endif  // GFX_PUBLIC_DISPLAY_LIST_BUILDER_H_
