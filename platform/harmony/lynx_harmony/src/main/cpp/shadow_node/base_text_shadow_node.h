// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_BASE_TEXT_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_BASE_TEXT_SHADOW_NODE_H_

#include <memory>
#include <string>
#include <vector>

#include "core/base/harmony/props_constant.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text/text_attributes.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_content.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ParagraphBuilderHarmony;

class BaseTextShadowNode : public ShadowNode, public ParagraphContent {
 public:
  BaseTextShadowNode(int sign, const std::string& tag);
  void OnPropsUpdate(char const* attr, lepus::Value const& value) override;
  ~BaseTextShadowNode() override = default;
  virtual bool IsRawText() const { return false; }
  virtual bool IsInlineTruncation() const { return false; }

  /**
   *
   * @param builder Builder object to construct the paragraph.
   * @param width Width of the reference box to create the shader effect. If no
   * effect needed, just pass 0. Unit is PX.
   * @param height Width of the reference box to create the shader effect. If no
   * effect needed, just pass 0. Unit is PX.
   *
   * The reference box will be removed once Harmony support update paint to a
   * TextStyle directly. If then, we don't need to build the shader effect
   * during the layout pass. We can update the paint once we got the layout
   * result during the rendering pass.
   */
  void AppendToParagraph(ParagraphBuilderHarmony& builder, float width,
                         float height) override;
  void OnAppendToParagraph(ParagraphBuilderHarmony& builder, float width,
                           float height) override;
  virtual void UpdateWordBreakType(starlight::WordBreakType word_break);
  ParagraphContent* AsParagraphContent() override { return this; }
  const TextStyleHarmony& GetStyle() const { return style_; }

  void LoadFontFamilyIfNeeded(const std::vector<std::string>& font_families,
                              FontCollectionHarmony* font_collection) const;
  TextProps* GetTextProps() {
    return text_props_.has_value() ? &(text_props_.value()) : nullptr;
  }
  // return false if has_placehodler inside
  bool BuildAttributedString(AttributedString& out);

  void OnContextReady() override;

 protected:
  void PrepareTextProps() {
    if (!text_props_.has_value()) {
      text_props_ = TextProps();
    }
  }
  void AddTextToBuilder(ParagraphBuilderHarmony& builder,
                        const std::string& text);
  const std::string& text() const { return text_; }

  std::optional<TextProps> text_props_;
  TextStyleHarmony style_;
  starlight::WordBreakType word_break_{starlight::WordBreakType::kNormal};

 private:
  std::string text_;
  void SetTextShadow(const lepus::Value& shadow);
  void SetColor(const lepus::Value& color);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_BASE_TEXT_SHADOW_NODE_H_
