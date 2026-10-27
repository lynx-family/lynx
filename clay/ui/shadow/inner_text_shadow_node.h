// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_INNER_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_INNER_TEXT_SHADOW_NODE_H_

#include <memory>
#include <optional>
#include <string>

#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/shadow/text_shadow_node.h"

namespace clay {

// this class just for soft keyboard view and video controller
class InnerTextShadowNode : public TextShadowNode {
 public:
  explicit InnerTextShadowNode(TextView* view)
      : InnerTextShadowNode(nullptr, "tappable-text", -1, view) {}

  InnerTextShadowNode(ShadowNodeOwner* owner, std::string tag, int id,
                      TextView* view)
      : TextShadowNode(owner, tag, id), text_view_(view) {
    text_style_ = std::make_optional<TextStyle>();
  }

  void ProcessParagraph(ShadowLayoutContextMeasure* context_measure,
                        std::unique_ptr<txt::Paragraph> paragraph);

  MeasureResult Measure(const MeasureConstraint& constraint) override;

  void TextLayout(LayoutContext* context) override;

 private:
  TextView* text_view_;
  enum UpdateFlag {
    // None means either there is no update or the text view's width/height
    // changed.
    // In this case, we don't need to re-create the text builder.
    kUpdateFlagNone = 0,
    kUpdateFlagStyle = 1,
    kUpdateFlagChildren = 1 << 1,
  };
  UpdateFlag update_flag_ = kUpdateFlagNone;
  float prev_layout_width_ = 0;
  int measured_width_;
  int measured_height_;
  MeasureConstraint constraint_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_INNER_TEXT_SHADOW_NODE_H_
