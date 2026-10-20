// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UI_UI_MARKDOWN_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UI_UI_MARKDOWN_H_

#include <memory>
#include <string>
#include <vector>

#include "markdown/markdown_event_listener.h"
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class MarkdownResourceLoaderHarmony;

class UIMarkdown : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
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
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  bool NeedWindowStateChangeEvent() const override { return true; }
  void OnEnterForeground() override;
  void OnEnterBackground() override;

 protected:
  UIMarkdown(LynxContext* context, int sign, const std::string& tag);

 private:
  void DetachMarkdownView();
  void LayoutMarkdownView();
  void RequestRendererFrame();
  lepus::Value GetTextRectsResult(
      const std::vector<serval::markdown::RectF>& boxes,
      const lepus::Value& args, bool include_handles);

  std::shared_ptr<serval::markdown::NativeMarkdownMeasurer> markdown_measurer_;
  std::shared_ptr<serval::markdown::MarkdownEventListener> event_listener_;
  std::shared_ptr<MarkdownResourceLoaderHarmony> resource_loader_;
  std::unique_ptr<serval::markdown::NativeServalMarkdownView> markdown_view_;
  serval::markdown::MeasureResult measured_size_{};
  bool renderer_frame_requested_{false};
  bool animation_paused_{false};
  bool in_background_{false};
  bool visible_in_window_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_MARKDOWN_SRC_MAIN_CPP_IMPL_UI_UI_MARKDOWN_H_
