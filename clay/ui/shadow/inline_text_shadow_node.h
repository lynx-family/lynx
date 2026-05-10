// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_INLINE_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_INLINE_TEXT_SHADOW_NODE_H_

#include <list>
#include <string>

#include "clay/shell/platform/common/text_range.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/shadow/base_text_shadow_node.h"

namespace clay {

using SendTextBoxesToUiThreadCallback =
    std::function<void(void* text_box_list)>;

class InlineTextShadowNode : public BaseTextShadowNode {
 public:
  InlineTextShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~InlineTextShadowNode() override;

  void TextLayout(LayoutContext* context) override;

  // Add a range [start, end).
  void AddTextRange(size_t start, size_t end);

  void LayoutRange(txt::Paragraph* paragraph);

  bool IsInlineTextShadowNode() override { return true; }

  bool IsVirtual() override { return true; }

  std::list<clay::TextRange> range_in_paragraph_;

 private:
  SendTextBoxesToUiThreadCallback text_boxex_callback_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_INLINE_TEXT_SHADOW_NODE_H_
