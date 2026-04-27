// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_TEXT_SHADOW_NODE_H_
#define CLAY_UI_SHADOW_TEXT_SHADOW_NODE_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/gfx/style/color_source.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/component/measurable.h"
#include "clay/ui/component/text/inline_emoji_bitmap.h"
#include "clay/ui/component/text/layout_client.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/shadow/base_text_shadow_node.h"
#include "clay/ui/shadow/inline_image_shadow_node.h"
#include "clay/ui/shadow/shadow_layout_context.h"
#include "clay/ui/shadow/shadow_node_owner.h"
#include "clay/ui/shadow/text_render.h"
#include "clay/ui/shadow/text_update_bundle.h"

namespace clay {

struct TextStroke;

class TextShadowNode : public BaseTextShadowNode, public CustomMeasurable {
 public:
  TextShadowNode(ShadowNodeOwner* owner, std::string tag, int id);
  ~TextShadowNode() override;

  bool IsTextShadowNode() override { return true; }
  CustomMeasurable* GetCustomMeasurable() override { return this; }

  void OnLayout(float width, TextMeasureMode width_mode, float height,
                TextMeasureMode height_mode,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders) override;

  MeasureResult Measure(const MeasureConstraint& constraint) override;

  ShadowLayoutContextMeasure CreateLayoutContext(
      const MeasureConstraint& constraint);

  void TextLayout(LayoutContext* context) override;

  void PreLayout(PreLayoutContext* context) override;

  void LayoutInlineText(txt::Paragraph* paragraph);

  void DidUpdateStyle();

  void ProcessParagraph(const MeasureConstraint& constraint,
                        ShadowLayoutContextMeasure* context_measure,
                        txt::Paragraph* paragraph);

  void MarkDirty() override;

  BaseView* FindViewByViewId(int id) { return owner_->FindViewByViewId(id); }

  void Align() override;

  // Only one line-height can take effect for a paragraph. So any line-height
  // set on inline-text-view are not reasonable. But for compatible reason,
  // we allow developer to set line-height on inline-text-view, and the final
  // line-height will be the maximum one.
  void UpdateLineHeight();

  void UpdateBundleData();

  void SetUpdateFlag(TextUpdateFlag flag);

  void AddUpdateFlag(TextUpdateFlag flag);

  double GetBaseline() const;

  void CreateGradientShaderMap(
      ShadowNode* node, txt::Paragraph* paragraph,
      std::map<int, std::shared_ptr<ColorSource>>& map,
      std::map<int, std::pair<size_t, size_t>>& range_map);

  void CreateTextStrokeMap(ShadowNode* node,
                           std::unordered_map<int, TextStroke>& map);

  ShadowNodeOwner* GetOwner() { return owner_; }

  void SetTextPaintAlign(TextAlignment align) { text_paint_align_ = align; }
  void SetInlineEmojiInfo(std::vector<InlineEmojiInfo>&& inline_emoji_info) {
    inline_emoji_info_ = std::move(inline_emoji_info);
  }
  TextAlignment GetResolvedTextAlign();
  void SetNeedSecondLayout(bool need_second_layout) {
    need_second_layout_ = need_second_layout;
  }

  void CreateTextBundle() { bundle_ = std::make_unique<TextUpdateBundle>(); }
  Bundle* GetTextBundle() { return bundle_.get(); }

 protected:
  txt::Paragraph* GetCacheParagraph();
  void SetCacheParagraph(std::unique_ptr<txt::Paragraph> paragraph);

 private:
  std::unique_ptr<TextRender> text_render_ = nullptr;
  bool need_second_layout_ = false;
  std::vector<InlineImageShadowNode*> inline_images_;
  std::vector<InlineViewShadowNode*> inline_views_;
  std::vector<InlineEmojiInfo> inline_emoji_info_;
  TextAlignment text_paint_align_ = TextAlignment::kLeft;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_TEXT_SHADOW_NODE_H_
