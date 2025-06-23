// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_SHADOW_NODE_H_

#include <memory>
#include <string>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/base_text_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_attributes.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ParagraphBuilderHarmony;
class FontCollectionHarmony;
class ParagraphHarmony;
class InlineTruncationShadowNode;
class TextShadowNode final : public BaseTextShadowNode,
                             public CustomMeasureFunc {
 public:
  TextShadowNode(int sign, const std::string& tag);
  static ShadowNode* Make(int sign, const std::string& tag) {
    return new TextShadowNode(sign, tag);
  }

  void OnLayoutBefore() override;
  void UpdateLayout(float left, float top, float width, float height) override;
  ~TextShadowNode() override;
  void OnContextReady() override;
  virtual bool IsTextShadowNode() const override { return true; }
  fml::RefPtr<fml::RefCountedThreadSafeStorage> getExtraBundle() override;
  LayoutResult Measure(float width, MeasureMode width_mode, float height,
                       MeasureMode height_mode, bool final_measure) override;
  void Align() override;

  void OnAppendToParagraph(ParagraphBuilderHarmony& builder, float width,
                           float height) override;
  void OnPropsUpdate(char const* attr, lepus::Value const& value) override;

 private:
  void HandleWhiteSpace();
  fml::RefPtr<ParagraphHarmony> HandleTextOverflowAndTruncation(
      fml::RefPtr<ParagraphHarmony> paragraph, float width,
      MeasureMode width_mode, float height, MeasureMode height_mode,
      bool final_measure);
  InlineTruncationShadowNode* FindInlineTruncationNode();
  int32_t FindTruncationLineIndex(ParagraphHarmony* paragraph,
                                  float layout_max_height);
  float ConstructTextWidthConstraint(MeasureMode width_mode, float width);
  float ConstructTextHeightConstraint(MeasureMode height_mode, float height);
  float CalcTextTranslateOffset(float layout_max_width, float text_max_width);
  void HandleMaxWidthMode(BaseTextShadowNode* shadow_node,
                          ParagraphHarmony* paragraph, float width,
                          MeasureMode width_mode, float layout_max_width,
                          float text_max_width);
  void UpdateLineHeight(BaseTextShadowNode* node, double line_height);
  void DispatchLayoutEvent();

  fml::RefPtr<ParagraphHarmony> ReBuildParagraph(
      ParagraphBuilderHarmony* builder, const LayoutResult result,
      const float layout_max_width, const float offset);
  fml::RefPtr<ParagraphHarmony> LayoutNode(BaseTextShadowNode* shadow_node,
                                           ParagraphBuilderHarmony* builder,
                                           float width, MeasureMode width_mode,
                                           float height,
                                           MeasureMode height_mode,
                                           bool final_measure);
  LayoutResult GetLayoutResult(ParagraphHarmony* paragraph, float width,
                               MeasureMode width_mode, float height,
                               MeasureMode height_mode);

  std::unique_ptr<ParagraphStyleHarmony> paragraph_style_;
  std::shared_ptr<FontCollectionHarmony> font_collection_;
  std::unique_ptr<ParagraphBuilderHarmony> paragraph_builder_;
  fml::RefPtr<ParagraphHarmony> paragraph_;
  InlineTruncationShadowNode* inline_truncation_shadow_node_;
  int32_t origin_char_count_{0};
  int32_t ellipsis_count_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_TEXT_SHADOW_NODE_H_
