// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/blur_view/ui_blur_view.h"

#include <deviceinfo.h>

#include "core/renderer/css/parser/css_string_parser.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {
static constexpr int kBackDropBlurSupportVersion = 15;

void UIBlurView::OnPropUpdate(const std::string &name,
                              const lepus::Value &value) {
  if (name == "blur-radius") {
    CSSStringParser parser = CSSStringParser::FromLepusString(value, {});
    CSSValue radius;
    parser.ParseLengthTo(radius);
    if (!radius.IsEmpty() && api_version_ >= kBackDropBlurSupportVersion) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          DrawNode(), NODE_BACKDROP_BLUR,
          radius.AsNumber() * context_->ScaledDensity());
    }
  } else {
    UIBase::OnPropUpdate(name, value);
  }
}

UIBlurView::UIBlurView(LynxContext *context, int sign, const std::string &tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag) {
  api_version_ = OH_GetSdkApiVersion();
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
