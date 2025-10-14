// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_RAW_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_RAW_TEXT_SHADOW_NODE_H_

#include <string>

#include "clay/ui/shadow/shadow_node.h"
#include "clay/ui/shadow/text_shadow_node.h"

namespace clay {

class RenderRawText;
class RawTextShadowNode : public ShadowNode {
 public:
  RawTextShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~RawTextShadowNode() override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void SetText(const std::string& text);
  void SetText(const std::u16string& text);

  void TextLayout(LayoutContext* context) override;
  bool IfNeedTextIndent();

  std::u16string Text() { return text_.substr(0, truncated_index_); }

  bool IsVirtual() override { return true; }

  bool IsRawTextShadowNode() override { return true; }

  std::u16string CollapsesWhitespaces(std::u16string text);

  TextShadowNode* FindTextShadowNodeAncestor();

  std::u16string ProcessWordBreakIfNeed(const std::u16string& text);

 private:
  std::u16string origin_text_;
  std::u16string text_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_RAW_TEXT_SHADOW_NODE_H_
