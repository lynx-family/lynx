// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_BOX_MODEL_RECORDER_H_
#define CORE_RENDERER_DOM_FRAGMENT_BOX_MODEL_RECORDER_H_

#include "base/include/flex_optional.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/layout_info.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

namespace lynx {
namespace tasm {

enum BoxModelType {
  kBoxModelTypeContent = 0,
  kBoxModelTypePadding,
  kBoxModelTypeBorder,
};

class BoxModelRecorder {
 public:
  BoxModelRecorder() = default;
  ~BoxModelRecorder() = default;

  void Reset();

  int32_t GetIndexOfBoxModel(BoxModelType, const LayoutInfoForDraw&,
                             DisplayListBuilder&);

 private:
  RoundedRectangle GenerateRoundedRectangle(BoxModelType,
                                            const LayoutInfoForDraw&);

  int32_t index_of_content_box_{-1};
  int32_t index_of_padding_box_{-1};
  int32_t index_of_border_box_{-1};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_BOX_MODEL_RECORDER_H_
