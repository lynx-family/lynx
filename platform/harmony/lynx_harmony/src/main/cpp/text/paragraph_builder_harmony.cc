// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_builder_harmony.h"

#include <renderer/css/css_style_utils.h>

#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/unicode_decode_utils.h"
namespace lynx {
namespace tasm {
namespace harmony {

void ParagraphBuilderHarmony::Reset() {
  if (builder_) {
    OH_Drawing_DestroyTypographyHandler(builder_);
  }
  builder_ = OH_Drawing_CreateTypographyHandler(
      paragraph_style_->GetRawStruct(), font_collection_->GetRawStruct());
  text_no_wrap_and_after_break_ = false;
  needs_rebuilding_ = false;
  char16_count_ = 0;
  placeholder_signs_.clear();
  cur_event_target_start_ = 0;
  event_target_stack_ = {};
  event_roots_.clear();
  effects_.reset();
}

void ParagraphBuilderHarmony::PushTextStyle(const TextStyleHarmony& style) {
  if (style.GradientColor()) {
    // Needs a rebuilding when we got a exact width and height to calculate the
    // gradient effect.
    needs_rebuilding_ = true;
  }
  if (!text_no_wrap_and_after_break_) {
    OH_Drawing_TypographyHandlerPushTextStyle(builder_, style.GetRawStruct());
    // Effects' lifetime should be as long as the Paragraph's.
    if (auto effect = style.GradientShaderEffect()) {
      // TODO(renzhongyue): extract the util function to a base public module.
      starlight::CSSStyleUtils::PrepareOptional(effects_);
      effects_->emplace_back(effect);
    }
  }
}

void ParagraphBuilderHarmony::PopTextStyle() {
  if (!text_no_wrap_and_after_break_) {
    OH_Drawing_TypographyHandlerPopTextStyle(builder_);
  }
}

void ParagraphBuilderHarmony::PushTextEventTarget(
    int32_t sign, LynxEventPropStatus event_through,
    LynxEventPropStatus ignore_focus) {
  cur_event_target_start_ = char16_count_;
  auto target = std::make_shared<TextEventTarget>(
      cur_event_target_start_, cur_event_target_start_ + 1, sign, event_through,
      ignore_focus);
  if (event_target_stack_.empty()) {
    event_roots_.emplace_back(target);
    event_target_stack_.push(target.get());
  } else {
    auto* parent = event_target_stack_.top();
    parent->AddChild(target);
    event_target_stack_.push(target.get());
  }
}

void ParagraphBuilderHarmony::PopTextEventTarget() {
  if (!event_target_stack_.empty()) {
    auto* target = event_target_stack_.top();
    target->SetEnd(char16_count_);
    event_target_stack_.pop();
  }
}

void ParagraphBuilderHarmony::AddText(const char* text) {
  if (char16_count_ >= max_char16_count_) return;
  auto text_byte_length = strlen(text);
  auto u16text = base::U8StringToU16({text, text_byte_length});
  int32_t text_char16_count = u16text.length();
  std::string text_string;
  if (max_char16_count_ - char16_count_ < text_char16_count) {
    auto u16text_splice = std::u16string_view(u16text).substr(
        0, max_char16_count_ - char16_count_);
    text_string = base::U16StringToU8(u16text_splice);
    text = text_string.data();
    text_char16_count = max_char16_count_ - char16_count_;
  }
  if (paragraph_style_->GetWhiteSpace() == starlight::WhiteSpaceType::kNowrap) {
    // white-space:nowrap, do not add text after first hard break
    if (!text_no_wrap_and_after_break_) {
      const auto* break_pos = strchr(text, '\n');
      if (break_pos == nullptr) {
        AddTextToBuilder(text);
        char16_count_ += text_char16_count;
      } else {
        std::string str_before_break(text, break_pos - text);
        AddTextToBuilder(str_before_break);
        text_no_wrap_and_after_break_ = true;
        char16_count_ += base::SizeOfUtf16(str_before_break);
      }
    }
  } else {
    AddTextToBuilder(text);
    char16_count_ += text_char16_count;
  }
}

void ParagraphBuilderHarmony::AddTextToBuilder(const std::string& text) {
  OH_Drawing_TypographyHandlerAddText(builder_, text.c_str());
  text_ += text;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
