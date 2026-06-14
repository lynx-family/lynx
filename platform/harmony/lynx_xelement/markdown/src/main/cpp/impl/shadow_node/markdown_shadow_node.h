// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_SHADOW_NODE_MARKDOWN_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_SHADOW_NODE_MARKDOWN_SHADOW_NODE_H_

#include <memory>
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

namespace serval {
namespace markdown {
class MarkdownView;
class NativeServalMarkdownView;
}  // namespace markdown
}  // namespace serval

namespace lynx {
namespace tasm {
namespace harmony {

class MarkdownViewBundle;
class MarkdownResourceLoaderHarmony;

class MarkdownShadowNode : public ShadowNode, public CustomMeasureFunc {
 public:
  static ShadowNode* Make(int sign, const std::string& tag) {
    return new MarkdownShadowNode(sign, tag);
  }

  MarkdownShadowNode(int sign, const std::string& tag);
  ~MarkdownShadowNode() override;

  void OnContextReady() override;
  fml::RefPtr<fml::RefCountedThreadSafeStorage> getExtraBundle() override;
  void OnPropsUpdate(const std::string& name,
                     const lepus::Value& value) override;
  void Destroy() override;

  LayoutResult Measure(float width, MeasureMode width_mode, float height,
                       MeasureMode height_mode, bool final_measure) override;
  void Align() override;

 private:
  void ApplyConfig(const std::string& name, const lepus::Value& value);
  std::shared_ptr<serval::markdown::NativeServalMarkdownView>
  EnsureMarkdownView();
  serval::markdown::MarkdownView* MarkdownView();

  std::shared_ptr<serval::markdown::NativeServalMarkdownView> markdown_view_;
  std::shared_ptr<MarkdownResourceLoaderHarmony> resource_loader_;
  fml::RefPtr<MarkdownViewBundle> bundle_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_SHADOW_NODE_MARKDOWN_SHADOW_NODE_H_
