// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_INLINE_SPEC_STYLES_H_
#define CLAY_UI_COMPONENT_TEXT_INLINE_SPEC_STYLES_H_

namespace clay {

// Styles like 'margin' won't be used by clay in most cases, because lynx
// will deal with it. But inline views need to handle margin itself.
// For code size consideration, use it independently.
class InlineSpecStyles {
 public:
  void SetInlineMargin(float margin_left, float margin_right) {
    margin_left_ = margin_left;
    margin_right_ = margin_right;
  }

  float margin_left() const { return margin_left_; }
  float margin_right() const { return margin_right_; }

 private:
  float margin_left_ = 0.f;
  float margin_right_ = 0.f;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_INLINE_SPEC_STYLES_H_
