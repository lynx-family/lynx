// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/layout_context.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "base/include/string/string_utils.h"
#include "clay/fml/logging.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/shadow/inline_image_shadow_node.h"

namespace clay {

static const std::u16string kDefaultEllipsis = u"\u2026";

PreLayoutContextText::PreLayoutContextText() = default;
PreLayoutContextText::~PreLayoutContextText() = default;

void PreLayoutContextText::CollectInlineImages(InlineImageView* image) {
  inline_images_.emplace_back(image);
}

std::vector<InlineImageView*> PreLayoutContextText::GetInlineImages() {
  return std::move(inline_images_);
}

void LayoutContextText::AddText(const std::u16string& text,
                                bool need_text_indent) {
  if (builder_) {
    std::u16string sub_text = text;
    if (max_length_.has_value() && *max_length_ > 0) {
      if (text.size() + TextSizeIncludingPlaceholders() > max_length_) {
        sub_text = text.substr(
            0, std::max(*max_length_ - TextSizeIncludingPlaceholders(),
                        static_cast<size_t>(0)));
        sub_text += kDefaultEllipsis;
      }
    }
    if (!need_text_indent) {
      builder_->AddText(sub_text);
      text_.append(sub_text);
    } else {
      char16_t newline_character = u'\u000A';
      std::u16string newline_string = std::u16string(1, newline_character);
      auto tokens = lynx::base::SplitString<std::u16string>(
          sub_text, newline_string, true);
      EnsureInitialTextIndentIfNeeded(need_text_indent);
      for (size_t i = 0; i < tokens.size(); i++) {
        if (i > 0) {
          text_.append(newline_string);
          builder_->AddText(newline_string);
          txt::PlaceholderRun placeholder(text_indent_.value_or(0), 0,
                                          txt::PlaceholderAlignment::kBaseline,
                                          txt::TextBaseline::kAlphabetic, 0);
          AddPlaceholder(placeholder);
        }
        text_.append(tokens[i]);
        builder_->AddText(tokens[i]);
      }
    }
  }
}

void LayoutContextText::EnsureInitialTextIndentIfNeeded(bool need_text_indent) {
  if (!builder_ || !need_text_indent || has_added_initial_text_indent_ ||
      text_.length() != 0) {
    return;
  }
  txt::PlaceholderRun placeholder(text_indent_.value_or(0), 0,
                                  txt::PlaceholderAlignment::kBaseline,
                                  txt::TextBaseline::kAlphabetic, 0);
  if (AddPlaceholder(placeholder) >= 0) {
    has_added_initial_text_indent_ = true;
  }
}

size_t LayoutContextText::TextSizeIncludingPlaceholdersInUtf32() {
  return lynx::base::U16StringToU32(text_).size() + placeholder_num_;
}

}  // namespace clay
