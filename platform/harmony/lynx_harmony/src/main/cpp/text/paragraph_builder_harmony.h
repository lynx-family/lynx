// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_BUILDER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_BUILDER_HARMONY_H_

#include <limits>
#include <list>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/text_event_target.h"

namespace lynx {
namespace tasm {
namespace harmony {

class ParagraphBuilderHarmony {
 public:
  ParagraphBuilderHarmony(ParagraphStyleHarmony* para_style,
                          FontCollectionHarmony* font_collection)
      : builder_(OH_Drawing_CreateTypographyHandler(
            para_style->GetRawStruct(), font_collection->GetRawStruct())),
        paragraph_style_(para_style),
        font_collection_(font_collection) {}

  ~ParagraphBuilderHarmony() {
    if (builder_) {
      OH_Drawing_DestroyTypographyHandler(builder_);
    }
  }

  void Reset();

  OH_Drawing_TypographyCreate* GetRawStruct() const { return builder_; }

  void PushTextStyle(const TextStyleHarmony& style);

  void AddText(const char* text);

  void AddPlaceholder(PlaceholderHarmony& placeholder, int32_t sign) {
    if (!text_no_wrap_and_after_break_ && char16_count_ < max_char16_count_) {
      OH_Drawing_TypographyHandlerAddPlaceholder(builder_,
                                                 placeholder.GetRawStruct());
      placeholder_signs_.emplace_back(sign);
      char16_count_++;
    }
  }

  void PopTextStyle();

  void PushTextEventTarget(int32_t sign, LynxEventPropStatus event_through,
                           LynxEventPropStatus ignore_focus);
  void PopTextEventTarget();

  fml::RefPtr<ParagraphHarmony> CreateParagraph(
      std::shared_ptr<FontCollectionHarmony>& font_collection, float width) {
    auto paragraph = fml::MakeRefCounted<ParagraphHarmony>(
        OH_Drawing_CreateTypography(builder_), font_collection,
        std::move(event_roots_), std::move(placeholder_signs_),
        std::move(effects_));
    if (paragraph_style_->GetTextIndent()) {
      float indent = paragraph_style_->GetTextIndent()->GetValue(width);
      paragraph->SetIndent(indent);
    }
    return paragraph;
  }

  FontCollectionHarmony* GetFontCollection() const { return font_collection_; }

  bool NeedsRebuilding() const { return needs_rebuilding_; }

  int32_t GetCharCount() const { return char16_count_; }

  void SetMaxCharCount(int32_t char_count) { max_char16_count_ = char_count; }

  int32_t GetMaxCharCount() const { return max_char16_count_; }

  const std::string& GetText() const { return text_; }

 private:
  void AddTextToBuilder(const std::string& text);

  OH_Drawing_TypographyCreate* builder_;
  ParagraphStyleHarmony* paragraph_style_;
  FontCollectionHarmony* font_collection_;

  // white-space:nowrap and after break, ignore all action when true
  bool text_no_wrap_and_after_break_{false};

  // Mark the paragraph needs rebuilding after first layout. (e.g inline-text
  // with gradient color needs to adjust the gradient shader sizes after we got
  // a measured size for this paragraphy)
  bool needs_rebuilding_ = false;

  // The following two member is to track the text range of the event target. In
  // {@link AddText} we will move the end position. When we push a event
  // target, we set {cur_event_target_end} to both {cur_event_target_start} and
  // start of the target. When we pop a event target,it's end will be set to
  // {cur_event_target_end}.
  size_t cur_event_target_start_ = 0;
  std::stack<TextEventTarget*> event_target_stack_;
  std::list<std::shared_ptr<TextEventTarget>> event_roots_;

  // inline truncation needs char count and max char count
  int32_t char16_count_{0};
  int32_t max_char16_count_{std::numeric_limits<int32_t>::max()};
  std::vector<int32_t> placeholder_signs_;
  std::optional<std::list<fml::RefPtr<ShaderEffect>>> effects_;

  std::string text_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_BUILDER_HARMONY_H_
