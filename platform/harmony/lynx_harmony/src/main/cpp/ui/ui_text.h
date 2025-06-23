// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_TEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_TEXT_H_

#include <native_drawing/drawing_canvas.h>
#include <node_api.h>

#include <string>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIText : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIText(context, sign, tag);
  }
  UIText(LynxContext* context, int sign, const std::string& tag);
  void Update(ParagraphHarmony* paragraph);
  void UpdateExtraData(
      const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) override;
  void OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) override;
  void Render(OH_Drawing_Canvas* canvas) const;
  void FrameDidChanged() override;
  EventTarget* HitTest(float point[2]) override;
  const std::string& GetAccessibilityLabel() const override;

 private:
  void UpdateInlineImageFrame();

  fml::RefPtr<ParagraphHarmony> paragraph_{nullptr};
  float translate_left_offset_{0.f};
  float translate_top_offset_{0.f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_TEXT_H_
