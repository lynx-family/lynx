// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <textra/paragraph.h>
#include <textra/style.h>

#include <cstdint>
#include <initializer_list>

#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#include "clay/ui/component/text/text_paragraph_builder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(TextParagraphBuilderTTTextTest, BaselineShiftPropagatesToTTTextStyle) {
  TextStyle parent;
  parent.font_size = 24.f;
  parent.align_type = VerticalAlignType::kVerticalAlignBaseline;
  parent.baseline_shift = 3.f;
  for (float shift : {-8.f, 0.f, 6.f}) {
    TextParagraphBuilder builder(true, parent);
    builder.PushStyle(parent);
    EXPECT_FLOAT_EQ(builder.PeekStyleForTesting().text_baseline_shift, 3.f);
    TextStyle child;
    child.font_size = 24.f;
    child.align_type = VerticalAlignType::kVerticalAlignBaseline;
    child.baseline_shift = shift;
    builder.PushStyle(child);
    EXPECT_EQ(builder.PeekStyleForTesting().align_type,
              tttext::CharacterVerticalAlignment::kBaseLine);
    EXPECT_FLOAT_EQ(builder.PeekStyleForTesting().text_baseline_shift, shift);
    builder.AddText(u"x");
    builder.Pop();
    EXPECT_FLOAT_EQ(builder.PeekStyleForTesting().text_baseline_shift, 3.f);
    builder.PushStyle(child);
    EXPECT_FLOAT_EQ(builder.PeekStyleForTesting().text_baseline_shift, shift);
    builder.AddText(u"y");
    builder.Pop();
    builder.Pop();
    auto paragraph = builder.builder_->Build();
    auto* tt_paragraph = static_cast<txt::ParagraphTTText*>(paragraph.get());
    for (uint32_t index : {0u, 1u}) {
      tttext::Style final_style;
      tt_paragraph->paragraph_->QueryStyle(index, &final_style);
      EXPECT_FLOAT_EQ(final_style.GetBaselineOffset(), shift);
    }
    EXPECT_FLOAT_EQ(child.baseline_shift.value(), shift);
  }

  TextParagraphBuilder inherited(true, parent);
  inherited.PushStyle(parent);
  TextStyle child_without_shift;
  child_without_shift.font_size = 24.f;
  child_without_shift.align_type = VerticalAlignType::kVerticalAlignBaseline;
  inherited.PushStyle(child_without_shift);
  EXPECT_FLOAT_EQ(inherited.PeekStyleForTesting().text_baseline_shift, 3.f);
}

}  // namespace clay
