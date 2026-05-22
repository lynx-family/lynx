// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/text_update_bundle.h"

#include "clay/ui/component/text/inline_text_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/rendering/text/render_inline_text.h"

namespace clay {

TextUpdateBundle::~TextUpdateBundle() {}

void TextUpdateBundle::UpdateExtraData(BaseView* view) {
  if (view && view->Is<TextView>() && paragraph_) {
    auto text_view = static_cast<TextView*>(view);
    for (auto info : info_) {
      auto view = text_view->page_view()->FindViewByViewId(info.id);
      auto parent_view =
          text_view->page_view()->FindViewByViewId(info.parent_id);
      if (!view || !parent_view) {
        break;
      }
      if (info.need_mount) {
        if (info.placeholder_index.value_or(-1) >= 0) {
          if (view->Is<InlineImageView>()) {
            text_view->PushInlineImageIndex(info.id,
                                            info.placeholder_index.value());
            static_cast<InlineImageView*>(view)->SetLocation(
                info.location.value());
          } else {
            text_view->PushInlineViewIndex(info.id,
                                           info.placeholder_index.value());
          }
        }

        if (info.view_style) {
          view->SetWidth(info.view_style->width);
          view->SetHeight(info.view_style->height);
          view->SetPaddings(
              info.view_style->padding_left, info.view_style->padding_top,
              info.view_style->padding_right, info.view_style->padding_bottom);
        }
        if (info.range_) {
          if (view->Is<InlineTextView>()) {
            auto render_object = view->render_object();
            static_cast<RenderInlineText*>(render_object)->ClearTextBox();
            for (auto range : info.range_.value()) {
              auto boxes = paragraph_->GetRectsForRange(
                  range.start(), range.end(),
                  txt::Paragraph::RectHeightStyle::kTight,
                  txt::Paragraph::RectWidthStyle::kTight);
              for (auto& box : boxes) {
                static_cast<RenderInlineText*>(render_object)
                    ->AddTextBox(box.rect);
              }
            }
            static_cast<InlineTextView*>(view)->SetTextRange(
                info.range_.value());
          }
        }
        if (!view->Parent()) {
          parent_view->AddChild(view);
        }
      } else {
        parent_view->RemoveChild(view);
      }
    }
    text_view->UpdateInlineImageInfo();
    text_view->SetParagraph(std::move(paragraph_), text_);
    text_view->GetRenderText()->SetTextStrokeMap(std::move(text_stroke_map_));
    text_view->GetRenderText()->SetGradientShaderMap(
        std::move(gradient_shader_map_), std::move(range_map_));
    text_view->GetRenderText()->SetTextPaintAlign(text_paint_align_);
    text_view->GetRenderText()->SetLineSpacingOffset(line_spacing_offset_);
  }
}
}  // namespace clay
