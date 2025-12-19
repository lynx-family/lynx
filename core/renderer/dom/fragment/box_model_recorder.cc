// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/box_model_recorder.h"

#include <algorithm>

namespace lynx {
namespace tasm {

void BoxModelRecorder::Reset() {
  index_of_content_box_ = -1;
  index_of_padding_box_ = -1;
  index_of_border_box_ = -1;
}

int32_t BoxModelRecorder::GetIndexOfBoxModel(
    BoxModelType box_model_type, const LayoutInfoForDraw& layout_info,
    DisplayListBuilder& display_list_builder) {
  int32_t* index = nullptr;

  switch (box_model_type) {
    case kBoxModelTypeContent:
      index = &index_of_content_box_;
      break;
    case kBoxModelTypePadding:
      index = &index_of_padding_box_;
      break;
    case kBoxModelTypeBorder:
      index = &index_of_border_box_;
      break;
  }

  if (*index < 0) {
    display_list_builder.RecordBoxModel(
        GenerateRoundedRectangle(box_model_type, layout_info), *index);
  }

  return *index;
}

RoundedRectangle BoxModelRecorder::GenerateRoundedRectangle(
    BoxModelType box_model_type, const LayoutInfoForDraw& layout_info) {
  switch (box_model_type) {
    case kBoxModelTypeContent:
      return layout_info.GenerateContentRectangle();
    case kBoxModelTypePadding:
      return layout_info.GeneratePaddingRectangle();
    case kBoxModelTypeBorder:
      return layout_info.GenerateBorderRectangle();
  }
}

}  // namespace tasm
}  // namespace lynx
