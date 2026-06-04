// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/text_update_bundle.h"

#include <algorithm>

#include "clay/ui/component/text/inline_text_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/rendering/text/render_inline_text.h"

namespace clay {

TextUpdateBundle::~TextUpdateBundle() {}

namespace {

FloatPoint GetTextPaintOffset(TextView* text_view, txt::Paragraph* paragraph,
                              TextAlignment text_paint_align,
                              double line_spacing_offset) {
  if (!text_view || !paragraph) {
    return {};
  }

  auto* render_text = text_view->GetRenderText();
  double x_offset = 0;
  if (text_paint_align == TextAlignment::kCenter) {
    x_offset =
        std::max(0.0, (render_text->ContentWidth() -
                       paragraph->GetLongestLine()) /
                          2);
  } else if (text_paint_align == TextAlignment::kRight) {
    x_offset =
        std::max(0.0, render_text->ContentWidth() - paragraph->GetLongestLine());
  }
  return render_text->PaintOffset() +
         FloatPoint(static_cast<float>(x_offset),
                    static_cast<float>(line_spacing_offset));
}

}  // namespace

void TextUpdateBundle::UpdateExtraData(BaseView* view) {
  if (view && view->Is<TextView>() && paragraph_) {
    auto text_view = static_cast<TextView*>(view);
    auto text_paint_offset = GetTextPaintOffset(
        text_view, paragraph_.get(), text_paint_align_, line_spacing_offset_);
    for (auto& info : info_) {
      auto page_view = text_view->page_view();
      if (!page_view || !page_view->GetViewContext()) {
        continue;
      }
      auto info_view = page_view->FindViewByViewId(info.id);
      auto parent_view = page_view->FindViewByViewId(info.parent_id);
      if (!info_view || !parent_view) {
        continue;
      }
      if (info.need_mount) {
        if (info.placeholder_index.value_or(-1) >= 0) {
          if (info_view->Is<InlineImageView>()) {
            text_view->PushInlineImageIndex(info.id,
                                            info.placeholder_index.value());
            static_cast<InlineImageView*>(info_view)->SetLocation(
                info.location.value());
          } else {
            text_view->PushInlineViewIndex(info.id,
                                           info.placeholder_index.value());
          }
        }

        if (info.view_style) {
          info_view->SetWidth(info.view_style->width);
          info_view->SetHeight(info.view_style->height);
          info_view->SetPaddings(info.view_style->padding_left,
                                 info.view_style->padding_top,
                                 info.view_style->padding_right,
                                 info.view_style->padding_bottom);
        }
        if (info.range_) {
          if (info_view->Is<InlineTextView>()) {
            auto render_object = info_view->render_object();
            static_cast<RenderInlineText*>(render_object)->ClearTextBox();
            if (info.use_inline_view_layout_box && info.view_style) {
              static_cast<RenderInlineText*>(render_object)
                  ->AddTextBox(skity::Rect::MakeLTRB(
                      0, 0, info.view_style->width, info.view_style->height));
              if (info.inline_view_paragraph) {
                static_cast<RenderInlineText*>(render_object)
                    ->SetParagraph(std::move(info.inline_view_paragraph),
                                   info.inline_view_text);
              }
            } else {
              for (auto range : info.range_.value()) {
                auto boxes = paragraph_->GetRectsForRange(
                    range.start(), range.end(),
                    txt::Paragraph::RectHeightStyle::kTight,
                    txt::Paragraph::RectWidthStyle::kTight);
                for (auto& box : boxes) {
                  box.rect.Offset(text_paint_offset.x(),
                                  text_paint_offset.y());
                  static_cast<RenderInlineText*>(render_object)
                      ->AddTextBox(box.rect);
                }
              }
            }
            static_cast<InlineTextView*>(info_view)->SetTextRange(
                info.range_.value());
          }
        }
        if (!info_view->Parent() && info.id > 0) {
          parent_view->AddChild(info_view);
        }
      } else {
        parent_view->RemoveChild(info_view);
      }
    }
    text_view->UpdateInlineImageInfo();
    text_view->SetInlineEmojiInfo(std::move(inline_emoji_info_));
    text_view->SetParagraph(std::move(paragraph_), text_);
    text_view->GetRenderText()->SetTextStrokeMap(std::move(text_stroke_map_));
    text_view->GetRenderText()->SetGradientShaderMap(
        std::move(gradient_shader_map_), std::move(range_map_));
    text_view->GetRenderText()->SetTextPaintAlign(text_paint_align_);
    text_view->GetRenderText()->SetLineSpacingOffset(line_spacing_offset_);
  }
}
}  // namespace clay
