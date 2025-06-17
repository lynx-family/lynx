// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_INPUT_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_INPUT_SHADOW_NODE_H_

#include <memory>
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/raw_text_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

static constexpr float INPUT_SHADOW_NODE_UNMEASURED_UI_HEIGHT = -1.0;

namespace lynx {
namespace tasm {
namespace harmony {

class InputShadowNode : public BaseTextShadowNode, public CustomMeasureFunc {
 public:
  InputShadowNode(int sign, const std::string& tag);
  static ShadowNode* Make(int sign, const std::string& tag) {
    return new InputShadowNode(sign, tag);
  }
  void OnPropsUpdate(char const* attr, lepus::Value const& value) override;
  LayoutResult Measure(float width, MeasureMode width_mode, float height,
                       MeasureMode height_mode, bool final_measure) override;
  void Align() override;
  ~InputShadowNode() override = default;
  void SetUIHeight(float height) { ui_height_ = height; };
  bool IsUIHeightUndefined() {
    return ui_height_ == INPUT_SHADOW_NODE_UNMEASURED_UI_HEIGHT;
  };
  int32_t UIHeight() const { return ui_height_; };

 private:
  float ui_height_{INPUT_SHADOW_NODE_UNMEASURED_UI_HEIGHT};
  std::unique_ptr<ParagraphStyleHarmony> paragraph_style_;
  std::unique_ptr<TextStyleHarmony> text_style_;
  std::shared_ptr<FontCollectionHarmony> font_collection_;
  std::unique_ptr<ParagraphBuilderHarmony> paragraph_builder_;
  fml::RefPtr<ParagraphHarmony> paragraph_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_INPUT_INPUT_SHADOW_NODE_H_
