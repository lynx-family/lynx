
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_LAYOUT_CONTEXT_H_
#define CLAY_UI_COMPONENT_TEXT_LAYOUT_CONTEXT_H_

#include <algorithm>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/ui/component/layout_controller.h"
#include "clay/ui/component/text/inline_emoji_bitmap.h"
#include "clay/ui/component/text/text_paragraph_builder.h"
#include "clay/ui/component/text/text_style.h"

namespace clay {

class InlineImageView;
class InlineImageShadowNode;

class PreLayoutContextText : public PreLayoutContext {
 public:
  PreLayoutContextText();
  virtual ~PreLayoutContextText();

  void CollectInlineImages(InlineImageView*);

  // Consume the collected inline images
  std::vector<InlineImageView*> GetInlineImages();

 private:
  std::vector<InlineImageView*> inline_images_;
};

class LayoutContextText : public LayoutContext {
 public:
  void SetBuilder(TextParagraphBuilder* builder) { builder_ = builder; }

  void SetMaxLength(uint32_t max_length) { max_length_ = max_length; }
  void SetTextIndent(size_t text_indent) { text_indent_ = text_indent; }

  TextParagraphBuilder* builder() { return builder_; }

  void AddText(const std::u16string& text, bool need_text_indent = false);
  void EnsureInitialTextIndentIfNeeded(bool need_text_indent);

  void ProcessTextWithIndent();

  // Used for simulate paddings/margins of inline views.
  int AddFakePlaceholder(float width) {
    txt::PlaceholderRun fake_placeholder;
    fake_placeholder.height = 1.0;
    fake_placeholder.width = width;
    return AddPlaceholder(fake_placeholder);
  }

  int AddPlaceholder(txt::PlaceholderRun& placeholder) {
    if (builder_) {
      if (max_length_ &&
          (max_length_.value() <= TextSizeIncludingPlaceholders())) {
        return -1;
      }
      builder_->AddPlaceholder(placeholder);
      return placeholder_num_++;
    }
    return -1;
  }

  size_t TextSizeIncludingPlaceholders() {
    return text_.size() + placeholder_num_;
  }

  size_t TextSizeIncludingPlaceholdersInUtf32();

  void AddInlineEmojiInfo(int placeholder_id, InlineEmojiBitmap&& bitmap) {
    inline_emoji_info_.push_back({placeholder_id, std::move(bitmap)});
  }

  std::vector<InlineEmojiInfo> TakeInlineEmojiInfo() {
    return std::move(inline_emoji_info_);
  }

 private:
  TextParagraphBuilder* builder_ = nullptr;
  std::u16string text_;
  size_t placeholder_num_ = 0;
  std::optional<uint32_t> max_length_;
  std::optional<size_t> text_indent_;
  bool has_added_initial_text_indent_ = false;
  std::vector<InlineEmojiInfo> inline_emoji_info_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_LAYOUT_CONTEXT_H_
