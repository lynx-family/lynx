// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_INLINE_TEXT_VIEW_H_
#define CLAY_UI_COMPONENT_TEXT_INLINE_TEXT_VIEW_H_

#include <list>
#include <utility>

#include "clay/shell/platform/common/text_range.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/inline_image_view.h"
#include "clay/ui/component/text/base_text_view.h"

namespace clay {

class InlineTextView : public WithTypeInfo<InlineTextView, BaseTextView> {
 public:
  InlineTextView(int id, PageView* page_view);
  ~InlineTextView() override;

  void SetTextRange(std::list<clay::TextRange>& range_in_paragraph) {
    range_in_paragraph_ = std::move(range_in_paragraph);
  }
  BaseView* GetDeepestViewInPos(txt::Paragraph::PositionWithAffinity text_pos);

 private:
  std::list<clay::TextRange> range_in_paragraph_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_INLINE_TEXT_VIEW_H_
