// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_TEXT_SPAN_H_
#define CLAY_UI_COMPONENT_EDITABLE_TEXT_SPAN_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/string/string_utils.h"
#include "clay/ui/component/text/text_paragraph_builder.h"
#include "clay/ui/component/text/text_style.h"

namespace clay {

class TextSpan {
 public:
  TextSpan(std::u16string text = {}, TextStyle text_style = {},
           std::vector<std::shared_ptr<TextSpan>> children = {});
  ~TextSpan() = default;
  void Build(TextParagraphBuilder& builder);

 private:
  std::u16string text_;
  TextStyle text_style_;
  std::vector<std::shared_ptr<TextSpan>> children_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_TEXT_SPAN_H_
