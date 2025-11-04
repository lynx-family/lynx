// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_BUILDER_H_
#define CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_BUILDER_H_

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx {
namespace tasm {

class DisplayListBuilder {
 public:
  DisplayListBuilder();
  ~DisplayListBuilder();

  DisplayListBuilder(const DisplayListBuilder&) = delete;
  DisplayListBuilder& operator=(const DisplayListBuilder&) = delete;
  DisplayListBuilder(DisplayListBuilder&&) = default;
  DisplayListBuilder& operator=(DisplayListBuilder&&) = default;

  // Begin a new fragment
  DisplayListBuilder& Begin(float x, float y, float width, float height);

  // End the current fragment
  DisplayListBuilder& End();

  // Fill with color
  DisplayListBuilder& Fill(uint32_t color);

  // Draw a view
  DisplayListBuilder& DrawView(int view_id);

  // Apply transform
  DisplayListBuilder& Transform(float a, float b, float c, float d, float e,
                                float f);

  // Apply clip
  DisplayListBuilder& Clip(float x, float y, float width, float height);

  // Build the final display list
  DisplayList Build();

  // Clear all items
  void Clear();

 private:
  DisplayList display_list_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_DISPLAY_LIST_BUILDER_H_
