// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UI_UI_MARKDOWN_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UI_UI_MARKDOWN_H_

#include <memory>
#include <string>

#include "markdown/platform/harmony/serval_markdown_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIMarkdown : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    InitMarkdownEnv(context);
    return new UIMarkdown(context, sign, tag);
  }

  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;
  void UpdateLayout(float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;
  void UpdateExtraData(
      const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) override;
  void OnDestroy() override;

 protected:
  UIMarkdown(LynxContext* context, int sign, const std::string& tag);

 private:
  static void InitMarkdownEnv(LynxContext* context);
  void DetachMarkdownView();

  std::shared_ptr<serval::markdown::NativeServalMarkdownView> markdown_view_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UI_UI_MARKDOWN_H_
