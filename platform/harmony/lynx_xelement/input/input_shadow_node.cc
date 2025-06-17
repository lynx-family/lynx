// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/input/input_shadow_node.h"

#include "base/include/float_comparison.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
static constexpr double INPUT_DEFAULT_FONT_SIZE = 14.0;

namespace lynx {
namespace tasm {
namespace harmony {

InputShadowNode::InputShadowNode(int sign, const std::string& tag)
    : BaseTextShadowNode(sign, tag) {
  paragraph_style_ = std::make_unique<ParagraphStyleHarmony>();
  text_style_ = std::make_unique<TextStyleHarmony>();
  text_style_->SetFontSize(INPUT_DEFAULT_FONT_SIZE);
  PrepareTextProps();
  text_props_->line_height = INPUT_DEFAULT_FONT_SIZE;
  font_collection_ = std::make_shared<FontCollectionHarmony>();
  paragraph_builder_ = std::make_unique<ParagraphBuilderHarmony>(
      paragraph_style_.get(), font_collection_.get());
  paragraph_builder_->AddText(" ");
  paragraph_builder_->PushTextStyle(*text_style_);
  SetCustomMeasureFunc(this);
}

void InputShadowNode::OnPropsUpdate(char const* attr,
                                    lepus::Value const& value) {}

LayoutResult InputShadowNode::Measure(float width, MeasureMode width_mode,
                                      float height, MeasureMode height_mode,
                                      bool final_measure) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, INPUT_SHADOW_NODE_MEASURE);
  AppendToParagraph(*paragraph_builder_, 0, 0);
  paragraph_ = paragraph_builder_->CreateParagraph(font_collection_, width);
  paragraph_->Layout(width * ScaleDensity());

  LayoutResult result{width, height, 0};

  if (base::FloatsNotEqual(ui_height_,
                           INPUT_SHADOW_NODE_UNMEASURED_UI_HEIGHT)) {
    result.height_ = ui_height_;
  } else {
    result.height_ =
        paragraph_->GetHeight() * ScaleDensity() - INPUT_DEFAULT_FONT_SIZE;
  }
  if (height_mode == MeasureMode::Definite) {
    result.height_ = height;
  }
  if (height_mode == MeasureMode::AtMost &&
      base::FloatsLarger(result.height_, height)) {
    result.height_ = height;
  }

  return result;
}

void InputShadowNode::Align() {}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
