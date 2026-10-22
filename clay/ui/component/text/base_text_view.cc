// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/base_text_view.h"

#include <utility>

#include "clay/ui/rendering/text/render_text.h"

namespace clay {

BaseTextView::BaseTextView(uint32_t id, std::string tag,
                           std::unique_ptr<RenderObject> render_object,
                           PageView* page_view)
    : WithTypeInfo(id, std::move(tag), std::move(render_object), page_view) {
#if FORCE_TEXTVIEW_FOCUSABLE
  SetFocusable(true);
#endif
}

BaseTextView::~BaseTextView() = default;

BaseView::MeaningfulContentStatus BaseTextView::GetMeaningfulContentStatus()
    const {
  auto* render_text = static_cast<const RenderText*>(render_object());
  return render_text->GetPainter() != nullptr &&
                 render_text->GetPainter()->CanPaint()
             ? MeaningfulContentStatus::kPresented
             : MeaningfulContentStatus::kPending;
}

}  // namespace clay
